#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "bridge_protocol.h"

// =============================================================================
//  UART link to the ESP32 A2DP bridge dongle (all-static, singleton).
//  poll() runs from the Arduino loop task; writePcm() is called from the
//  audio task - a mutex keeps frame writes from interleaving.
// =============================================================================

struct BridgeScanItem {
    String name;
    int8_t rssi = 0;
    uint8_t addr[6] = {0};
};

class BridgeLink {
public:
    static void begin();
    static void poll();          // call from loop task, ~10 ms cadence

    static bool isAlive() { return alive; }
    static bool isConnected() { return connected; }

    static void sendScanStart();
    static void sendScanStop();
    static void sendConnect(const uint8_t addr[6]);
    static void sendDisconnect();
    static void sendVolume(uint8_t vol01to100);
    static void sendPing();
    static void writePcm(const uint8_t *data, size_t len);  // audio task only

    // ---- event callbacks (invoked from the loop task inside poll()) ----
    static void onScanItem(void (*cb)(const BridgeScanItem &)) { scanItemCb = cb; }
    static void onScanDone(void (*cb)()) { scanDoneCb = cb; }
    static void onStatus(void (*cb)(uint8_t state, const char *name)) { statusCb = cb; }
    static void onAvrcpKey(void (*cb)(uint8_t key)) { avrcpCb = cb; }
    static void onAliveChanged(void (*cb)(bool isAlive)) { aliveCb = cb; }

private:
    static void handleFrame();
    static void sendCmd(uint8_t type, const void *payload, uint16_t len);

    static HardwareSerial serial;  // UART1
    static BrpParser parser;
    static SemaphoreHandle_t writeMutex;

    static volatile bool alive;
    static volatile bool connected;
    static uint32_t lastPingMs;
    static uint32_t lastPongMs;
    static uint32_t pingSeq;
    static uint32_t pongMisses;

    static void (*scanItemCb)(const BridgeScanItem &);
    static void (*scanDoneCb)();
    static void (*statusCb)(uint8_t, const char *);
    static void (*avrcpCb)(uint8_t);
    static void (*aliveCb)(bool);
};
