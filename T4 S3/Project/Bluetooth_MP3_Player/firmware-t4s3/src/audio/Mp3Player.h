#pragma once

#include <Arduino.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "PcmRing.h"
#include "Resampler.h"
#include "libhelix-mp3/mp3dec.h"
#include "../Id3Parser.h"

// =============================================================================
//  SD-card MP3 playback engine.
//
//  A dedicated FreeRTOS task decodes MP3 (Helix) from the SPI SD card,
//  converts every file to 44.1 kHz stereo and streams the PCM either to the
//  A2DP bridge dongle over UART (back-pressured by the UART buffer) or to an
//  optional local I2S DAC. All transport commands go through a command queue
//  so the UI thread never touches the decoder.
// =============================================================================

struct TrackItem {
    String path;      // full path on SD, e.g. /fs/Music/song.mp3
    String title;     // filename stem until ID3 tags are parsed on play
    String artist;
};

enum class PlayerOutput : uint8_t { Bridge = 0, Speaker };

enum PlayerCmdId : uint8_t {
    PCMD_NONE = 0,
    PCMD_PLAY_INDEX,
    PCMD_TOGGLE_PAUSE,
    PCMD_NEXT,
    PCMD_PREV,
    PCMD_SEEK_MS,
    PCMD_STOP,
    PCMD_SET_OUTPUT,
    PCMD_SET_VOLUME,
};

struct PlayerCmd {
    PlayerCmdId id;
    int32_t a;
};

class Mp3Player {
public:
    bool begin(const std::vector<TrackItem> *library);

    // ---- transport API (thread-safe) ----
    void playIndex(int index);
    void togglePause();
    void next();
    void prev();
    void seekMs(uint32_t ms);
    void stop();
    void setVolume(uint8_t vol01to100);   // speaker path; BT uses AVRCP on bridge
    void setOutput(PlayerOutput out);

    // ---- state getters (poll-safe) ----
    bool isPlaying() const { return state != ST_IDLE; }
    bool isPaused() const { return paused; }
    int currentIndex() const { return index; }
    uint32_t elapsedMs() const { return elapsed; }
    uint32_t durationMs() const { return duration; }
    bool trackChanged() {
        bool dirty = metaDirty;
        metaDirty = false;
        return dirty;
    }

    // track metadata for UI (copy under mutex)
    void currentTrack(TrackItem &out);

private:
    enum State : uint8_t { ST_IDLE, ST_PLAYING };

    static void taskTrampoline(void *arg);
    void taskLoop();
    bool processCmds(bool waitMs);
    bool openTrack(int idx);
    void closeTrack();
    bool decodeOneFrame();
    void pump();
    void pushPcm(int16_t *pcm, size_t bytes);
    void finishTrack();     // EOF / fatal error -> advance
#if ENABLE_LOCAL_SPEAKER
    void initSpeaker();
#endif

    const std::vector<TrackItem> *lib = nullptr;
    QueueHandle_t cmdQueue = nullptr;
    SemaphoreHandle_t metaMutex = nullptr;

    // decoder workspace (allocated once, task-private)
    PcmRing ring;
    Resampler resampler;
    uint8_t *readBuf = nullptr;    // MP3_READ_BUF_SIZE
    int16_t *pcmBuf = nullptr;     // one decoded frame
    uint8_t *rsBuf = nullptr;      // resampler output
    HMP3Decoder helix = nullptr;

    File file;
    bool fileOpen = false;
    bool fileEof = false;
    bool pumpPrimed = false;
    size_t inBytes = 0;
    uint32_t audioStartOff = 0;
    int decodeErrors = 0;

    // playback state (task-private)
    State state = ST_IDLE;
    int index = -1;
    bool paused = false;
    uint32_t pendingSeekMs = 0xFFFFFFFF;
    uint64_t samplesEmitted = 0;

    // shared with UI (volatile)
    volatile uint32_t elapsed = 0;
    volatile uint32_t duration = 0;
    volatile bool metaDirty = false;
    TrackItem meta;                // guarded by metaMutex

    PlayerOutput out = PlayerOutput::Bridge;
    uint8_t volumePct = 80;
};
