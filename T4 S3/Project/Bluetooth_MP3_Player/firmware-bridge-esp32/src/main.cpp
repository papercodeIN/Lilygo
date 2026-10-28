// =============================================================================
//  ESP32 A2DP bridge dongle for the LilyGo T4-S3 Bluetooth MP3 Player.
//
//  The T4-S3 (ESP32-S3) has Bluetooth LE only - no Bluetooth Classic, so it
//  cannot speak A2DP to headphones. This dongle is the radio:
//    - receives decoded PCM (44.1 kHz stereo s16) over UART from the T4-S3
//    - scans Bluetooth Classic and reports devices to the T4-S3 UI
//    - connects to the device the user picked and streams via A2DP/SBC
//    - forwards AVRCP keys (headset play/pause/next...) back to the T4-S3
//
//  Wiring: T4-S3 GPIO21 -> GPIO16 (RX2), T4-S3 GPIO47 <- GPIO17 (TX2), GND-GND
// =============================================================================

#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "bridge_protocol.h"

#ifndef BRIDGE_RX_PIN
#define BRIDGE_RX_PIN 16
#endif
#ifndef BRIDGE_TX_PIN
#define BRIDGE_TX_PIN 17
#endif

BluetoothA2DPSource a2dp;
HardwareSerial linkSerial(2);

// ---- PCM ring: producer = loop() (UART parse), consumer = A2DP task -------
static uint8_t *pcmRing = nullptr;
static const size_t PCM_RING_SIZE = 80 * 1024;
static volatile size_t ringHead = 0, ringTail = 0;
static portMUX_TYPE ringMux = portMUX_INITIALIZER_UNLOCKED;

static inline size_t ringUsed() {
    size_t h = ringHead, t = ringTail;
    return (h >= t) ? (h - t) : (PCM_RING_SIZE - t + h);
}

static void ringPush(const uint8_t *data, size_t len) {
    // single-core chip: plain reads are fine; overflow drops (live audio)
    size_t h = ringHead, t = ringTail;
    size_t usedBytes = (h >= t) ? (h - t) : (PCM_RING_SIZE - t + h);
    size_t freeBytes = PCM_RING_SIZE - 1 - usedBytes;
    if (len > freeBytes) len = freeBytes;
    if (!len) return;
    size_t contig = PCM_RING_SIZE - h;
    if (contig > len) contig = len;
    memcpy(pcmRing + h, data, contig);
    if (len > contig) memcpy(pcmRing, data + contig, len - contig);
    ringHead = (h + len) % PCM_RING_SIZE;
}

// A2DP task asks for PCM; pad silence when the link is starved.
static int32_t onPcmRequest(uint8_t *data, int32_t len) {
    size_t got = 0;
    size_t h = ringHead, t = ringTail;
    while (got < (size_t)len && t != h) {
        size_t contig = (t < h) ? (h - t) : (PCM_RING_SIZE - t);
        if (contig > (size_t)len - got) contig = (size_t)len - got;
        memcpy(data + got, pcmRing + t, contig);
        got += contig;
        t = (t + contig) % PCM_RING_SIZE;
    }
    taskENTER_CRITICAL(&ringMux);
    ringTail = t;
    taskEXIT_CRITICAL(&ringMux);
    if (got < (size_t)len) memset(data + got, 0, len - got);
    return len;
}

// ---- bridge state -----------------------------------------------------------
static volatile uint8_t linkState = BRP_LINK_IDLE;
static volatile bool wantScan = true;        // keep discovery alive while idle
static volatile bool discoveryActive = false;
static volatile bool pendingConnect = false; // ssid_callback should select this
static uint8_t pendingAddr[6] = {0};
static char lastName[40] = "(none)";
static uint8_t volumePct = 80;               // 0..100 as sent by the T4-S3
static uint32_t restartAtMs = 0;             // discovery watchdog
static SemaphoreHandle_t evtMutex = nullptr;

static void sendEvt(uint8_t type, const void *payload, uint16_t len) {
    if (!evtMutex) return;
    if (xSemaphoreTake(evtMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
    struct W {
        HardwareSerial &s;
        size_t operator()(const void *p, size_t n) { return s.write((const uint8_t *)p, n); }
    } w{linkSerial};
    brpSendFrame(w, type, payload, len);
    xSemaphoreGive(evtMutex);
}

static void setStatus(uint8_t state, const char *name) {
    linkState = state;
    if (name && name[0]) strncpy(lastName, name, sizeof(lastName) - 1);
    uint8_t buf[48] = {0};
    size_t n = strlen(lastName);
    if (n > 40) n = 40;
    buf[0] = state;
    memcpy(buf + 1, lastName, n);
    sendEvt(BRP_EVT_STATUS, buf, n + 1);
}

// ---- A2DP callbacks ---------------------------------------------------------
static bool onSsidFound(const char *ssid, esp_bd_addr_t address, int rssi) {
    // Called for every audio device found during Classic discovery.
    uint8_t buf[64] = {0};
    buf[0] = (uint8_t)(int8_t)rssi;
    memcpy(buf + 1, address, 6);
    const char *name = (ssid && ssid[0]) ? ssid : "(unnamed)";
    strncpy((char *)buf + 7, name, 56);
    sendEvt(BRP_EVT_SCAN_ITEM, buf, 7 + strlen((char *)buf + 7) + 1);

    if (pendingConnect && memcmp(address, pendingAddr, 6) == 0) {
        pendingConnect = false;
        setStatus(BRP_LINK_CONNECTING, name);
        return true;  // select this device -> library connects
    }
    return false;  // keep scanning
}

static void onDiscoveryState(esp_bt_gap_discovery_state_t mode) {
    discoveryActive = (mode == ESP_BT_GAP_DISCOVERY_STARTED);
    if (!discoveryActive && wantScan && linkState != BRP_LINK_CONNECTED) {
        restartAtMs = millis() + 800;  // re-trigger discovery shortly
    }
}

static void onConnState(esp_a2d_connection_state_t state, void *) {
    switch (state) {
        case ESP_A2D_CONNECTION_STATE_CONNECTED:
            wantScan = false;
            setStatus(BRP_LINK_CONNECTED, lastName);
            a2dp.set_volume((uint8_t)((uint32_t)volumePct * 127 / 100));
            break;
        case ESP_A2D_CONNECTION_STATE_CONNECTING:
            setStatus(BRP_LINK_CONNECTING, lastName);
            break;
        case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
            wantScan = true;
            restartAtMs = millis() + 800;
            setStatus(BRP_LINK_DISCONNECTED, lastName);
            break;
        default:
            break;
    }
}

static void onAvrcpKey(uint8_t key, bool isReleased) {
    if (!isReleased) return;  // act on release only
    uint8_t k = key;
    sendEvt(BRP_EVT_AVRCP_KEY, &k, 1);
}

// ---- discovery lifecycle ----------------------------------------------------
// The library only scans while start() runs its connect state machine, so a
// clean end()/start() cycle is used to (re)launch discovery.
static void restartDiscoveryCycle() {
    a2dp.end(true);
    delay(300);
    a2dp.start();
}

// ---- UART command handling --------------------------------------------------
static BrpParser parser;

static void handleCmd() {
    switch (parser.type) {
        case BRP_CMD_PING: {
            if (parser.payloadLen < 4) break;
            uint8_t buf[64] = {0};
            memcpy(buf, parser.payload, 4);          // seq echo
            buf[4] = linkState;
            strncpy((char *)buf + 5, lastName, 56);
            sendEvt(BRP_EVT_PONG, buf, 5 + strlen((char *)buf + 5) + 1);
            break;
        }
        case BRP_CMD_SCAN_START:
            wantScan = true;
            pendingConnect = false;
            if (!discoveryActive && linkState != BRP_LINK_CONNECTED) {
                restartAtMs = millis() + 100;
            }
            break;
        case BRP_CMD_SCAN_STOP:
            wantScan = false;
            break;
        case BRP_CMD_CONNECT: {
            if (parser.payloadLen < 6) break;
            memcpy(pendingAddr, parser.payload, 6);
            pendingConnect = true;
            if (!discoveryActive) restartAtMs = millis() + 100;
            break;
        }
        case BRP_CMD_DISCONNECT:
            wantScan = true;
            setStatus(BRP_LINK_DISCONNECTED, lastName);
            restartAtMs = millis() + 200;  // reset cycle resumes discovery
            break;
        case BRP_CMD_VOLUME:
            if (parser.payloadLen >= 1) {
                volumePct = parser.payload[0] > 100 ? 100 : parser.payload[0];
                if (linkState == BRP_LINK_CONNECTED) {
                    a2dp.set_volume((uint8_t)((uint32_t)volumePct * 127 / 100));
                }
            }
            break;
        case BRP_CMD_PCM:
            ringPush(parser.payload, parser.payloadLen);
            break;
        default:
            break;
    }
}

// ---- setup / loop -------------------------------------------------------------
void setup() {
    Serial.begin(115200);  // USB debug
    evtMutex = xSemaphoreCreateMutex();

    pcmRing = (uint8_t *)malloc(PCM_RING_SIZE);
    if (!pcmRing) {
        Serial.println("PCM ring alloc failed");
        while (true) delay(1000);
    }

    linkSerial.setRxBufferSize(8192);
    linkSerial.setTxBufferSize(2048);
    linkSerial.begin(2000000, SERIAL_8N1, BRIDGE_RX_PIN, BRIDGE_TX_PIN);

    a2dp.set_local_name("T4S3-Audio-Bridge");
    a2dp.set_auto_reconnect(false);
    a2dp.set_data_callback(onPcmRequest);
    a2dp.set_ssid_callback(onSsidFound);
    a2dp.set_discovery_mode_callback(onDiscoveryState);
    a2dp.set_on_connection_state_changed(onConnState);
    a2dp.set_avrc_passthru_command_callback(onAvrcpKey);
    a2dp.start();  // boots Bluedroid and begins discovery

    Serial.println("Bridge up: scanning for A2DP sinks");
}

void loop() {
    // UART frames in
    while (linkSerial.available()) {
        uint8_t b = (uint8_t)linkSerial.read();
        if (parser.feed(b)) handleCmd();
    }

    // discovery watchdog: keep scanning whenever we are not connected
    if (restartAtMs && millis() >= restartAtMs) {
        restartAtMs = 0;
        if (wantScan && !discoveryActive && linkState != BRP_LINK_CONNECTED) {
            restartDiscoveryCycle();
        }
    }

    delay(4);
}
