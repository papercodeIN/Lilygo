#include "Mp3Player.h"
#include "config.h"
#include "bridge_protocol.h"
#include "../bt/BridgeLink.h"

#if ENABLE_LOCAL_SPEAKER
#include <driver/i2s.h>
#endif

#ifndef ERR_MP3_NONE
#define ERR_MP3_NONE 0
#endif

// -----------------------------------------------------------------------------
// lifecycle
// -----------------------------------------------------------------------------
bool Mp3Player::begin(const std::vector<TrackItem> *library) {
    lib = library;
    if (!ring.init(PCM_RING_SIZE)) {
        log_e("PCM ring alloc failed");
        return false;
    }
    readBuf = (uint8_t *)malloc(MP3_READ_BUF_SIZE);
    pcmBuf = (int16_t *)malloc(1152 * 2 * sizeof(int16_t) * 2);
    rsBuf = (uint8_t *)heap_caps_malloc(RS_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!readBuf || !pcmBuf || !rsBuf) {
        log_e("audio workspace alloc failed");
        return false;
    }
    helix = MP3InitDecoder();
    cmdQueue = xQueueCreate(16, sizeof(PlayerCmd));
    metaMutex = xSemaphoreCreateMutex();
    if (!cmdQueue || !metaMutex || !helix) return false;
    xTaskCreatePinnedToCore(taskTrampoline, "mp3task", AUDIO_TASK_STACK_SIZE,
                            this, AUDIO_TASK_PRIORITY, nullptr, AUDIO_TASK_CORE);
    return true;
}

// -----------------------------------------------------------------------------
// transport API
// -----------------------------------------------------------------------------
void Mp3Player::playIndex(int idx) {
    PlayerCmd c = {PCMD_PLAY_INDEX, idx};
    xQueueSend(cmdQueue, &c, pdMS_TO_TICKS(100));
}
void Mp3Player::togglePause() {
    PlayerCmd c = {PCMD_TOGGLE_PAUSE, 0};
    xQueueSend(cmdQueue, &c, 0);
}
void Mp3Player::next() {
    PlayerCmd c = {PCMD_NEXT, 0};
    xQueueSend(cmdQueue, &c, 0);
}
void Mp3Player::prev() {
    PlayerCmd c = {PCMD_PREV, 0};
    xQueueSend(cmdQueue, &c, 0);
}
void Mp3Player::seekMs(uint32_t ms) {
    PlayerCmd c = {PCMD_SEEK_MS, (int32_t)ms};
    xQueueSend(cmdQueue, &c, 0);
}
void Mp3Player::stop() {
    PlayerCmd c = {PCMD_STOP, 0};
    xQueueSend(cmdQueue, &c, pdMS_TO_TICKS(100));
}
void Mp3Player::setVolume(uint8_t vol01to100) {
    PlayerCmd c = {PCMD_SET_VOLUME, vol01to100};
    xQueueSend(cmdQueue, &c, 0);
}
void Mp3Player::setOutput(PlayerOutput o) {
    PlayerCmd c = {PCMD_SET_OUTPUT, (int32_t)o};
    xQueueSend(cmdQueue, &c, 0);
}

void Mp3Player::currentTrack(TrackItem &out) {
    if (xSemaphoreTake(metaMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        out = meta;
        xSemaphoreGive(metaMutex);
    }
}

// -----------------------------------------------------------------------------
// task body
// -----------------------------------------------------------------------------
void Mp3Player::taskTrampoline(void *arg) {
    ((Mp3Player *)arg)->taskLoop();
    vTaskDelete(nullptr);
}

void Mp3Player::taskLoop() {
    for (;;) {
        bool active = (state == ST_PLAYING && !paused && fileOpen);
        processCmds(!active);  // sleep in the queue-wait only when idle

        if (state != ST_PLAYING || paused) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (!fileOpen) {
            if (!openTrack(index)) {
                finishTrack();
                continue;
            }
        }

        if (pendingSeekMs != 0xFFFFFFFF) {
            uint32_t target = pendingSeekMs;
            pendingSeekMs = 0xFFFFFFFF;
            uint32_t dur = duration;
            if (dur && target + 1000 >= dur) target = (dur > 1500) ? dur - 1500 : 0;
            // approximate byte position from average bitrate
            uint32_t audioBytes = file.size() - audioStartOff;
            uint64_t off = dur ? ((uint64_t)audioBytes * target) / dur : 0;
            file.seek(audioStartOff + (size_t)off);
            inBytes = 0;
            fileEof = false;
            decodeErrors = 0;
            ring.reset();
            pumpPrimed = false;
            elapsed = target;
            samplesEmitted = (uint64_t)target * OUT_SAMPLE_RATE / 1000ULL;
        }

        // decode while the ring has room, then drain to the output
        if (ring.freeSpace() > MP3_READ_BUF_SIZE / 2) {
            if (!decodeOneFrame()) {
                finishTrack();
                continue;
            }
        }
        pump();
    }
}

bool Mp3Player::processCmds(bool waitMs) {
    PlayerCmd c;
    bool got = false;
    TickType_t wait = waitMs ? pdMS_TO_TICKS(20) : 0;
    while (xQueueReceive(cmdQueue, &c, got ? 0 : wait) == pdTRUE) {
        got = true;
        bool restart = false;  // need a fresh openTrack()
        switch (c.id) {
            case PCMD_PLAY_INDEX:
                if (state == ST_IDLE || index != c.a) {
                    closeTrack();
                    index = c.a;
                    state = ST_PLAYING;
                    paused = false;
                    restart = true;
                }
                break;
            case PCMD_TOGGLE_PAUSE:
                if (state == ST_PLAYING) paused = !paused;
                break;
            case PCMD_NEXT:
                if (state == ST_PLAYING && lib && !lib->empty()) {
                    closeTrack();
                    index = (index + 1) % (int)lib->size();
                    paused = false;
                    restart = true;
                }
                break;
            case PCMD_PREV:
                if (state == ST_PLAYING && lib && !lib->empty()) {
                    if (elapsed > 3000) {
                        pendingSeekMs = 0;  // restart current track
                    } else {
                        closeTrack();
                        index = (index - 1 + (int)lib->size()) % (int)lib->size();
                        paused = false;
                        restart = true;
                    }
                }
                break;
            case PCMD_SEEK_MS:
                pendingSeekMs = (uint32_t)c.a;
                break;
            case PCMD_STOP:
                closeTrack();
                state = ST_IDLE;
                paused = false;
                index = -1;
                break;
            case PCMD_SET_OUTPUT: {
                PlayerOutput o = (PlayerOutput)c.a;
#if ENABLE_LOCAL_SPEAKER
                if (o == PlayerOutput::Speaker) initSpeaker();
                out = o;
#else
                (void)o;
                out = PlayerOutput::Bridge;
#endif
                ring.reset();
                pumpPrimed = false;
                break;
            }
            case PCMD_SET_VOLUME:
                volumePct = (uint8_t)c.a;
                break;
            default:
                break;
        }
        if (restart) {
            elapsed = 0;
            duration = 0;
            samplesEmitted = 0;
            decodeErrors = 0;
        }
    }
    return got;
}

// -----------------------------------------------------------------------------
// file / track handling
// -----------------------------------------------------------------------------
bool Mp3Player::openTrack(int idx) {
    if (!lib || idx < 0 || idx >= (int)lib->size()) return false;
    const TrackItem &t = (*lib)[idx];

    Mp3Meta m;
    if (!Id3Parser::parse(t.path, m)) {
        log_e("cannot open %s", t.path.c_str());
        return false;
    }

    if (xSemaphoreTake(metaMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        meta = t;
        if (!m.title.isEmpty()) meta.title = m.title;
        if (!m.artist.isEmpty()) meta.artist = m.artist;
        xSemaphoreGive(metaMutex);
        metaDirty = true;
    }
    duration = m.durationMs;
    audioStartOff = m.audioStart;

    file = SD.open(t.path, FILE_READ);
    if (!file) return false;
    if (m.audioStart) file.seek(m.audioStart);
    fileOpen = true;
    fileEof = false;
    inBytes = 0;
    decodeErrors = 0;
    pumpPrimed = false;
    ring.reset();
    resampler.configure(44100, 2);  // real format detected on first frame
    return true;
}

void Mp3Player::closeTrack() {
    if (fileOpen) file.close();
    fileOpen = false;
    ring.reset();
    pumpPrimed = false;
}

void Mp3Player::finishTrack() {
    // auto-advance to next track (wraps around)
    closeTrack();
    if (lib && !lib->empty() && state == ST_PLAYING) {
        index = (index + 1) % (int)lib->size();
        elapsed = 0;
        duration = 0;
        samplesEmitted = 0;
        paused = false;
    } else {
        state = ST_IDLE;
        index = -1;
    }
}

// -----------------------------------------------------------------------------
// decoding
// -----------------------------------------------------------------------------
bool Mp3Player::decodeOneFrame() {
    // top-up the input buffer from the file
    if (inBytes < MP3_READ_BUF_SIZE / 2 && !fileEof) {
        size_t toRead = MP3_READ_BUF_SIZE - inBytes;
        int got = file.read(readBuf + inBytes, toRead);
        if (got <= 0) fileEof = true;
        else inBytes += got;
    }
    if (inBytes == 0) return false;  // clean EOF

    uint8_t *ptr = readBuf;
    int bytesLeft = (int)inBytes;

    int sync = MP3FindSyncWord(ptr, bytesLeft);
    if (sync < 0) {
        // nothing usable in the buffer - drop it and read on
        inBytes = 0;
        return !fileEof;
    }
    ptr += sync;
    bytesLeft -= sync;

    int err = MP3Decode(helix, &ptr, &bytesLeft, pcmBuf, 0);
    size_t consumed = (size_t)(ptr - readBuf);  // decoder advances ptr on error too
    if (err != ERR_MP3_NONE) {
        decodeErrors++;
        if (consumed == 0) consumed = 1;  // skip the false sync
        memmove(readBuf, readBuf + consumed, inBytes - consumed);
        inBytes -= consumed;
        if (decodeErrors > 24) return false;  // give up on this file
        return true;
    }
    decodeErrors = 0;

    MP3FrameInfo fi;
    MP3GetLastFrameInfo(helix, &fi);
    size_t pcmBytes = (size_t)fi.outputSamps * sizeof(int16_t);
    resampler.configure((uint32_t)fi.samprate, (uint8_t)fi.nChans);

    memmove(readBuf, readBuf + consumed, inBytes - consumed);
    inBytes -= consumed;

    pushPcm(pcmBuf, pcmBytes);
    return true;
}

void Mp3Player::pushPcm(int16_t *pcm, size_t bytes) {
#if ENABLE_LOCAL_SPEAKER
    const bool softVolume = (out == PlayerOutput::Speaker && volumePct < 100);
#else
    const bool softVolume = false;
#endif
    if (resampler.isPassthrough()) {
        if (softVolume) {
            int32_t g = volumePct;
            for (size_t i = 0; i < bytes / 2; i++)
                pcm[i] = (int16_t)(((int32_t)pcm[i] * g) / 100);
        }
        if (bytes > ring.freeSpace()) bytes = ring.freeSpace() & ~3;
        if (bytes) ring.push((uint8_t *)pcm, bytes, pdMS_TO_TICKS(400));
        return;
    }

    size_t n = resampler.convert(pcm, bytes, rsBuf, RS_BUF_SIZE);
    if (!n) return;
    if (softVolume) {
        int32_t g = volumePct;
        int16_t *s = (int16_t *)rsBuf;
        for (size_t i = 0; i < n / 2; i++) s[i] = (int16_t)(((int32_t)s[i] * g) / 100);
    }
    ring.push(rsBuf, n, pdMS_TO_TICKS(400));
}

#if ENABLE_LOCAL_SPEAKER
void Mp3Player::initSpeaker() {
    static bool installed = false;
    if (installed) return;
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate = OUT_SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.dma_buf_count = 8;
    cfg.dma_buf_len = 512;
    cfg.use_apll = false;
    if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) == ESP_OK) {
        i2s_pin_config_t pins = {};
        pins.bck_io_num = SPEAKER_I2S_BCLK_PIN;
        pins.ws_io_num = SPEAKER_I2S_LRC_PIN;
        pins.data_out_num = SPEAKER_I2S_DOUT_PIN;
        pins.data_in_num = I2S_PIN_NO_CHANGE;
        i2s_set_pin(I2S_NUM_0, &pins);
        i2s_zero_dma_buffer(I2S_NUM_0);
        installed = true;
    }
}
#endif

// -----------------------------------------------------------------------------
// output pump
// -----------------------------------------------------------------------------
void Mp3Player::pump() {
    // wait for prefill before starting a fresh stream
    if (!pumpPrimed) {
        if (ring.used() >= PCM_RING_PREFILL || fileEof) pumpPrimed = true;
        else return;
    }

    static uint8_t chunk[PCM_FRAME_CHUNK];
    for (;;) {
        size_t n = ring.pop(chunk, sizeof(chunk));
        if (n == 0) {
            pumpPrimed = false;  // drained - let the decoder refill
            break;
        }
        samplesEmitted += n / 4;
        elapsed = (uint32_t)(samplesEmitted * 1000ULL / OUT_SAMPLE_RATE);

        if (out == PlayerOutput::Bridge) {
            BridgeLink::writePcm(chunk, n);  // blocks while the UART saturates
        }
#if ENABLE_LOCAL_SPEAKER
        else {
            size_t written = 0;
            while (written < n) {
                size_t w = 0;
                if (i2s_write(I2S_NUM_0, chunk + written, n - written, &w,
                              pdMS_TO_TICKS(200)) != ESP_OK || !w)
                    break;
                written += w;
            }
        }
#endif
        vTaskDelay(0);  // yield to same-core tasks between chunks
    }
}
