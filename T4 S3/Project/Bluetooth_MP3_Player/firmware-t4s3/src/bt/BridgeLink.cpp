#include "BridgeLink.h"
#include "config.h"

HardwareSerial BridgeLink::serial(1);
BrpParser BridgeLink::parser;
SemaphoreHandle_t BridgeLink::writeMutex = nullptr;

volatile bool BridgeLink::alive = false;
uint32_t BridgeLink::lastPingMs = 0;
uint32_t BridgeLink::lastPongMs = 0;
uint32_t BridgeLink::pingSeq = 0;

void (*BridgeLink::scanItemCb)(const BridgeScanItem &) = nullptr;
void (*BridgeLink::scanDoneCb)() = nullptr;
void (*BridgeLink::statusCb)(uint8_t, const char *) = nullptr;
void (*BridgeLink::avrcpCb)(uint8_t) = nullptr;
void (*BridgeLink::aliveCb)(bool) = nullptr;

void BridgeLink::begin() {
    writeMutex = xSemaphoreCreateMutex();
    serial.setRxBufferSize(2048);
    serial.setTxBufferSize(32 * 1024);  // smooths PCM bursts; drains at 2 Mbaud
    serial.begin(BRIDGE_UART_BAUD, SERIAL_8N1, BRIDGE_UART_RX_PIN, BRIDGE_UART_TX_PIN);
    parser.reset();
    lastPingMs = millis();
}

void BridgeLink::poll() {
    while (serial.available()) {
        uint8_t b = (uint8_t)serial.read();
        if (parser.feed(b)) handleFrame();
    }

    uint32_t now = millis();
    if (now - lastPingMs >= BRIDGE_PING_PERIOD_MS) {
        lastPingMs = now;
        sendPing();
        if (alive && (now - lastPongMs > (uint32_t)BRIDGE_PING_PERIOD_MS * BRIDGE_PING_MISSES)) {
            alive = false;
            if (aliveCb) aliveCb(false);
        }
    }
}

void BridgeLink::handleFrame() {
    switch (parser.type) {
        case BRP_EVT_PONG: {
            if (parser.payloadLen < 5) break;  // seq + link state
            lastPongMs = millis();
            if (!alive) {
                alive = true;
                if (aliveCb) aliveCb(true);
            }
            break;
        }
        case BRP_EVT_SCAN_ITEM: {
            if (parser.payloadLen < 8) break;
            BridgeScanItem item;
            item.rssi = (int8_t)parser.payload[0];
            memcpy(item.addr, parser.payload + 1, 6);
            // bounded: never assume the sender NUL-terminated the name
            item.name = String((const char *)parser.payload + 7,
                               strnlen((const char *)parser.payload + 7,
                                       parser.payloadLen - 7));
            if (item.name.isEmpty()) item.name = "(unnamed)";
            if (scanItemCb) scanItemCb(item);
            break;
        }
        case BRP_EVT_SCAN_DONE:
            if (scanDoneCb) scanDoneCb();
            break;
        case BRP_EVT_STATUS: {
            if (parser.payloadLen < 1) break;
            uint8_t state = parser.payload[0];
            char name[64] = {0};  // bounded, always NUL-terminated
            if (parser.payloadLen > 1) {
                size_t n = parser.payloadLen - 1;
                if (n > sizeof(name) - 1) n = sizeof(name) - 1;
                strncpy(name, (const char *)parser.payload + 1, n);
            }
            if (statusCb) statusCb(state, name);
            break;
        }
        case BRP_EVT_AVRCP_KEY:
            if (parser.payloadLen >= 1 && avrcpCb) avrcpCb(parser.payload[0]);
            break;
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
// senders
// -----------------------------------------------------------------------------
void BridgeLink::sendCmd(uint8_t type, const void *payload, uint16_t len) {
    if (!writeMutex) return;
    if (xSemaphoreTake(writeMutex, pdMS_TO_TICKS(250)) != pdTRUE) return;
    struct W {
        HardwareSerial &s;
        size_t operator()(const void *p, size_t n) { return s.write((const uint8_t *)p, n); }
    } w{serial};
    brpSendFrame(w, type, payload, len);
    xSemaphoreGive(writeMutex);
}

void BridgeLink::sendPing() {
    uint32_t seq = ++pingSeq;
    sendCmd(BRP_CMD_PING, &seq, 4);
}

void BridgeLink::sendScanStart() { sendCmd(BRP_CMD_SCAN_START, nullptr, 0); }

void BridgeLink::sendConnect(const uint8_t addr[6]) { sendCmd(BRP_CMD_CONNECT, addr, 6); }
void BridgeLink::sendDisconnect() { sendCmd(BRP_CMD_DISCONNECT, nullptr, 0); }

void BridgeLink::sendVolume(uint8_t vol01to100) {
    uint8_t v = vol01to100 > 100 ? 100 : vol01to100;
    sendCmd(BRP_CMD_VOLUME, &v, 1);
}

void BridgeLink::writePcm(const uint8_t *data, size_t len) {
    if (!writeMutex || !alive) return;
    if (xSemaphoreTake(writeMutex, pdMS_TO_TICKS(500)) != pdTRUE) return;
    size_t off = 0;
    while (off < len) {
        uint16_t chunk = (uint16_t)((len - off > PCM_FRAME_CHUNK) ? PCM_FRAME_CHUNK : (len - off));
        struct W {
            HardwareSerial &s;
            size_t operator()(const void *p, size_t n) { return s.write((const uint8_t *)p, n); }
        } w{serial};
        if (brpSendFrame(w, BRP_CMD_PCM, data + off, chunk) == 0) break;
        off += chunk;
    }
    xSemaphoreGive(writeMutex);
}
