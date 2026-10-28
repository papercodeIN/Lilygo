#include "PowerManager.h"
#include "config.h"

static const BattPoint kCurve[] = {
    {4200, 100}, {4110, 95}, {4080, 90}, {4020, 85}, {3980, 80}, {3950, 75},
    {3910, 70},  {3870, 65}, {3840, 60}, {3800, 55}, {3770, 50}, {3740, 45},
    {3710, 40},  {3680, 35}, {3650, 30}, {3610, 25}, {3570, 20}, {3530, 15},
    {3480, 10},  {3420, 6},  {3350, 3},  {3000, 0},
};

static uint8_t mvToPercent(uint16_t mv) {
    const size_t n = sizeof(kCurve) / sizeof(kCurve[0]);
    if (mv >= kCurve[0].mv) return 100;
    for (size_t i = 0; i + 1 < n; i++) {
        if (mv >= kCurve[i + 1].mv) {
            // linear interpolation between the two points
            uint16_t span = kCurve[i].mv - kCurve[i + 1].mv;
            uint16_t up = mv - kCurve[i + 1].mv;
            uint8_t p = kCurve[i + 1].pct +
                        (uint8_t)(((uint32_t)up * (kCurve[i].pct - kCurve[i + 1].pct)) / span);
            return p;
        }
    }
    return 0;
}

void PowerManager::begin(LilyGo_Class &device) {
    dev = &device;
    dev->SY.setChargeTargetVoltage(CHARGE_TARGET_MV);
    dev->SY.setChargerConstantCurr(CHARGE_CURRENT_MA);
    dev->SY.setPrechargeCurr(64);
    dev->SY.enableCharge();
    dev->SY.disableStatLed();  // save power on battery
    poll();
    started = true;
}

void PowerManager::poll() {
    if (!dev) return;
    uint16_t mv = dev->SY.getBattVoltage();
    if (mv < 2000 || mv > 5000) return;  // ADC glitch guard

    battMv = (battMv == 0) ? mv : (uint16_t)(((uint32_t)battMv * 3 + mv) / 4);  // EMA

    bool chg = dev->SY.isCharging();
    bool vbus = dev->SY.isVbusIn();
    charging = chg;
    if (vbus != vbusIn) vbusIn = vbus;

    // charging holds the curve near 4.0 V; clamp display to a sensible floor
    uint8_t p = mvToPercent(battMv);
    if (chg && p < 5) p = 5;
    if (p != pct) pct = p;
}
