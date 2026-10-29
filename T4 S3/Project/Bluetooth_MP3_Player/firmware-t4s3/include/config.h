#pragma once

// =============================================================================
//  LilyGo T4-S3 Bluetooth MP3 Player - build-time configuration
// =============================================================================

// ---------------------------------------------------------------------------
// UART link to the ESP32 A2DP bridge dongle.
//
// The ESP32-S3 chip on this board has Bluetooth 5 LE only - it physically
// cannot speak Bluetooth Classic (A2DP) or LE Audio, so a tiny companion
// ESP32 dongle acts as the Bluetooth radio (see top-level README).
//
// Both GPIOs are free pins on the T4-S3 30-pin 1.27 mm expansion header
// (verified against the official schematic). Any other free header pin
// works too - just change the numbers here.
// ---------------------------------------------------------------------------
#define BRIDGE_UART_TX_PIN      21      // T4-S3 TX  ->  bridge RX (GPIO16)
#define BRIDGE_UART_RX_PIN      47      // T4-S3 RX  <-  bridge TX (GPIO17)
#define BRIDGE_UART_BAUD        2000000 // carries decoded PCM, must be >= 2 Mbps

// ---------------------------------------------------------------------------
// Optional: local 3.5 mm/speaker output via a MAX98357A I2S amplifier board.
// Default OFF - enable by setting to 1 and wiring:
//   MAX98357A BCLK <- GPIO38,  LRC <- GPIO39,  DIN <- GPIO40
//   Vin <- T4-S3 VDD3V3 pin,  GND <-> GND     (SD pin of MAX98357A: leave
//   unconnected = 9 dB gain, or wire to GND for 12 dB / VDD for 6 dB)
// When enabled, "Onboard speaker" appears in the Bluetooth device list.
// ---------------------------------------------------------------------------
#define ENABLE_LOCAL_SPEAKER    0
#define SPEAKER_I2S_BCLK_PIN    38
#define SPEAKER_I2S_LRC_PIN     39
#define SPEAKER_I2S_DOUT_PIN    40

// ---------------------------------------------------------------------------
// Audio pipeline
// ---------------------------------------------------------------------------
#define AUDIO_TASK_CORE         1
#define AUDIO_TASK_PRIORITY     5
#define AUDIO_TASK_STACK_SIZE   12288
#define PCM_RING_SIZE           (256 * 1024)   // PSRAM ring between decoder and UART
#define PCM_RING_PREFILL        (64 * 1024)    // start pumping only after this much
#define MP3_READ_BUF_SIZE       (8 * 1024)     // file -> decoder linear buffer
#define PCM_FRAME_CHUNK         1200           // bytes per PCM_DATA frame on UART
#define RS_BUF_SIZE             (26 * 1024)    // resampler workspace; worst case 8 kHz mono needs ~25.4 KB stereo out per decoded frame

// Output sample format (A2DP / SBC standard rate)
#define OUT_SAMPLE_RATE         44100
#define OUT_CHANNELS            2

// ---------------------------------------------------------------------------
// UI / power behaviour
// ---------------------------------------------------------------------------
#define UI_TASK_CORE            0
#define UI_TASK_PRIORITY        2
#define UI_TASK_STACK_SIZE      32768
#define UI_POLL_PERIOD_MS       100

#define DIM_BRIGHTNESS_LEVEL    60     // after DIM_TIMEOUT_MS idle
#define DIM_TIMEOUT_MS          15000
#define SCREEN_OFF_BRIGHTNESS   0      // after OFF_TIMEOUT_MS idle
#define OFF_TIMEOUT_MS          60000

#define BATTERY_POLL_MS         5000
#define LOW_BATTERY_PERCENT     10
#define SHUTDOWN_BATTERY_PERCENT 1     // hibernate below this

// Device discovery
#define LOCAL_SPEAKER_NAME      "Onboard speaker"  // pseudo-device when ENABLE_LOCAL_SPEAKER=1
#define SCAN_DEDUPE_MAX         32     // devices kept in the UI list
#define BRIDGE_PING_PERIOD_MS   1500
#define BRIDGE_PING_TIMEOUT_MS  900
#define BRIDGE_PING_MISSES      3

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------
#define SD_MOUNT_PATH           "/fs"   // mount point created by the LilyGo library
#define MAX_TRACKS              500
#define MAX_DIR_DEPTH           1       // root + one level of sub-folders

// Battery charge profile (SY6970)
#define CHARGE_TARGET_MV        4208    // 4.208 V termination - gentle on LiPo
#define CHARGE_CURRENT_MA       704     // conservative for small cells
