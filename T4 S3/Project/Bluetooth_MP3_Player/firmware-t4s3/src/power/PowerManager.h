#pragma once

#include <Arduino.h>
#include <LilyGo_AMOLED.h>

// =============================================================================
//  Battery + charging supervision via the onboard SY6970 charger IC.
//  poll() is called from the UI task timer (shares the Wire bus with touch).
// =============================================================================

class PowerManager {
public:
    void begin(LilyGo_Class &device);

    void poll();  // ~ every BATTERY_POLL_MS

    uint8_t percent() const { return pct; }
    uint16_t voltageMv() const { return battMv; }
    bool isCharging() const { return charging; }
    bool isPlugged() const { return vbusIn; }

private:
    LilyGo_Class *dev = nullptr;
    uint8_t pct = 0;
    uint16_t battMv = 0;
    bool charging = false;
    bool vbusIn = false;
    bool started = false;
};

// LiPo open-circuit voltage -> percent curve (voltage under light load)
struct BattPoint {
    uint16_t mv;
    uint8_t pct;
};
