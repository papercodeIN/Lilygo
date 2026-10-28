// =============================================================================
//  LilyGo T4-S3 Bluetooth MP3 Player - main firmware
//
//  Boots the RM690B0 AMOLED + CST226SE touch + SY6970 charger via the LilyGo
//  library, then hands over to App (UI, SD MP3 playback, UART link to the
//  ESP32 A2DP bridge dongle that talks to your headphones/speaker).
// =============================================================================

#include <Arduino.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include "App.h"

static LilyGo_Class amoled;

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[T4-S3] Bluetooth MP3 Player booting...");

    // Display + PMU + touch + SD (mounted at /fs) - one call.
    // disable_state_led=true keeps the charger LED off (battery friendly).
    bool ok = amoled.beginAMOLED_241(false, true);
    if (!ok) {
        Serial.println("[T4-S3] Board init FAILED - halting");
        while (true) delay(1000);
    }
    amoled.setRotation(1);  // portrait 450x600 (rotation 0 is landscape on this panel)
    Serial.println("[T4-S3] Display ready");

    beginLvglHelper(amoled);
    Serial.println("[T4-S3] LVGL ready");

    App.begin(amoled);
    Serial.println("[T4-S3] App started");
}

void loop() {
    App.tick();  // bridge UART polling + protocol parsing
    delay(8);
}
