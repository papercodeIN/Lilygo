#pragma once

// =============================================================================
//  UART protocol between the T4-S3 main device and the ESP32 A2DP bridge.
//
//  Frame layout (little endian):
//    [0xA5][0x96][TYPE u8][LEN u16][PAYLOAD ...][CKSUM u8]
//    CKSUM = sum8(TYPE + LEN_LO + LEN_HI + every payload byte)
//
//  NOTE: this file is duplicated in firmware-bridge-esp32/include/.
//  Keep both copies identical.
// =============================================================================

#define BRP_SYNC0            0xA5
#define BRP_SYNC1            0x96
#define BRP_MAX_PAYLOAD      1600

// ---- T4-S3 -> bridge -------------------------------------------------------
#define BRP_CMD_PING         0x01   // payload: u32 seq
#define BRP_CMD_SCAN_START   0x02   // payload: none
#define BRP_CMD_SCAN_STOP    0x03   // payload: none
#define BRP_CMD_CONNECT      0x04   // payload: 6-byte BT address
#define BRP_CMD_DISCONNECT   0x05   // payload: none
#define BRP_CMD_VOLUME       0x06   // payload: u8 volume 0..100
#define BRP_CMD_PCM          0x86   // payload: raw PCM, S16 LE interleaved stereo @44100

// ---- bridge -> T4-S3 -------------------------------------------------------
#define BRP_EVT_PONG         0x81   // payload: u32 seq, u8 link_state, name (nul-terminated)
#define BRP_EVT_SCAN_ITEM    0x82   // payload: i8 rssi, 6-byte addr, name (nul-terminated)
#define BRP_EVT_SCAN_DONE    0x83   // payload: none
#define BRP_EVT_STATUS       0x84   // payload: u8 state, name (nul-terminated)
#define BRP_EVT_AVRCP_KEY    0x85   // payload: u8 key

// ---- bridge link / A2DP state ----------------------------------------------
enum BrpLinkState : uint8_t {
    BRP_LINK_IDLE = 0,       // bridge is up, no discovery running
    BRP_LINK_SEARCHING,      // Classic BT discovery in progress
    BRP_LINK_CONNECTING,
    BRP_LINK_CONNECTED,
    BRP_LINK_DISCONNECTED,   // was connected, peer went away
};

// AVRCP passthrough key codes (esp_avrc_pt_cmd_t subset)
#define BRP_AVRCP_PLAY       0x44
#define BRP_AVRCP_PAUSE      0x45
#define BRP_AVRCP_FORWARD    0x4B
#define BRP_AVRCP_BACKWARD   0x4C
#define BRP_AVRCP_VOL_UP     0x41
#define BRP_AVRCP_VOL_DOWN   0x42

// ---- helpers ----------------------------------------------------------------
static inline uint8_t brpChecksum(const uint8_t *payload, uint16_t len, uint8_t type) {
    uint8_t sum = type + (uint8_t)(len & 0xFF) + (uint8_t)(len >> 8);
    for (uint16_t i = 0; i < len; i++) sum += payload[i];
    return sum;
}

// Writer side: push a complete frame. Returns bytes written or 0.
// `writeFn` must be a blocking byte writer (returns number of bytes written).
template <typename WriteFn>
static inline size_t brpSendFrame(WriteFn writeFn, uint8_t type,
                                  const void *payload, uint16_t len) {
    if (len > BRP_MAX_PAYLOAD) return 0;
    uint8_t header[5] = {BRP_SYNC0, BRP_SYNC1, type,
                         (uint8_t)(len & 0xFF), (uint8_t)(len >> 8)};
    size_t n = writeFn(header, 5);
    if (n < 5) return 0;
    if (len) {
        n = writeFn(payload, len);
        if (n < len) return 0;
    }
    uint8_t ck = brpChecksum((const uint8_t *)payload, len, type);
    return (writeFn(&ck, 1) == 1) ? (size_t)(len + 6) : 0;
}

// Reader side: incremental frame parser. Feed bytes one at a time; when a full
// frame is available, parse() returns true and out_* are filled (out_payload
// points at the parser's internal buffer, valid until the next feed()).
class BrpParser {
public:
    void reset() {
        state = ST_SYNC0;
        payloadLen = 0;
    }

    bool feed(uint8_t b) {
        switch (state) {
            case ST_SYNC0:
                if (b == BRP_SYNC0) state = ST_SYNC1;
                break;
            case ST_SYNC1:
                state = (b == BRP_SYNC1) ? ST_TYPE : ST_SYNC0;
                break;
            case ST_TYPE:
                type = b;
                state = ST_LEN0;
                break;
            case ST_LEN0:
                lenLo = b;
                state = ST_LEN1;
                break;
            case ST_LEN1:
                payloadLen = (uint16_t)lenLo | ((uint16_t)b << 8);
                if (payloadLen > BRP_MAX_PAYLOAD) {  // bogus length - resync
                    reset();
                    break;
                }
                idx = 0;
                runningCk = (uint8_t)(type + lenLo + b);
                state = payloadLen ? ST_PAYLOAD : ST_CHECK;
                break;
            case ST_PAYLOAD:
                payload[idx++] = b;
                runningCk += b;
                if (idx >= payloadLen) state = ST_CHECK;
                break;
            case ST_CHECK:
                state = ST_SYNC0;
                return (b == runningCk);
        }
        return false;
    }

    uint8_t type = 0;
    uint16_t payloadLen = 0;
    uint8_t payload[BRP_MAX_PAYLOAD];

private:
    enum State : uint8_t { ST_SYNC0, ST_SYNC1, ST_TYPE, ST_LEN0, ST_LEN1, ST_PAYLOAD, ST_CHECK };
    State state = ST_SYNC0;
    uint8_t lenLo = 0;
    uint16_t idx = 0;
    uint8_t runningCk = 0;
};
