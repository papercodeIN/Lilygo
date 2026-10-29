#pragma once

#include <Arduino.h>

// =============================================================================
//  Tiny linear resampler: converts decoded MP3 PCM (any rate, mono or stereo)
//  into 44.1 kHz 16-bit interleaved stereo - the fixed format of the A2DP link.
//  Quality is "good enough" for casual listening; 44.1 kHz stereo files pass
//  through byte-for-byte.
// =============================================================================
class Resampler {
public:
    // Returns true if the format changed and internal state was reset.
    bool configure(uint32_t inRate, uint8_t inChannels) {
        if (inRate == 0 || inChannels == 0 || inChannels > 2) {
            inRate = 44100;
            inChannels = 2;
        }
        if (inRate == curRate && inChannels == curCh) return false;
        curRate = inRate;
        curCh = inChannels;
        passthrough = (inRate == 44100 && inChannels == 2);
        // Q16.16 input-frames consumed per output frame
        step = ((uint64_t)inRate << 16) / 44100ULL;
        pos = 0;
        prevL = prevR = 0;
        return true;
    }

    bool isPassthrough() const { return passthrough; }

    // in: interleaved s16, curCh channels. out: interleaved s16 stereo.
    // Returns output bytes written. outCap must cover the worst case: 8 kHz
    // mono input upsamples ~5.5x, so a full 1152-sample decode frame needs
    // ~25.4 KB of stereo output (see RS_BUF_SIZE in config.h).
    size_t convert(const int16_t *in, size_t inBytes, uint8_t *out, size_t outCap) {
        size_t inFrames = inBytes / (curCh * sizeof(int16_t));
        if (!inFrames || passthrough) {
            if (inBytes > outCap) inBytes = outCap & ~3;
            memcpy(out, in, inBytes);
            return inBytes;
        }

        int16_t *o = (int16_t *)out;
        size_t outFrames = 0;
        size_t maxOut = outCap / 4;

        while (outFrames < maxOut) {
            uint32_t whole = (uint32_t)(pos >> 16);
            if (whole >= inFrames) {          // consumed this buffer
                pos -= (uint64_t)inFrames << 16;
                break;
            }
            uint32_t frac = (uint32_t)(pos & 0xFFFF);
            // frame[whole] with index 0 being the carry sample from last call
            int16_t curL = (whole == 0) ? prevL : in[(whole - 1) * curCh];
            int16_t curR = (whole == 0) ? prevR : ((curCh == 2) ? in[(whole - 1) * 2 + 1] : curL);
            int16_t nxtL = in[whole * curCh];
            int16_t nxtR = (curCh == 2) ? in[whole * 2 + 1] : nxtL;
            *o++ = curL + (int16_t)(((int32_t)(nxtL - curL) * (int32_t)frac) >> 16);
            *o++ = curR + (int16_t)(((int32_t)(nxtR - curR) * (int32_t)frac) >> 16);
            outFrames++;
            pos += step;
        }
        prevL = in[(inFrames - 1) * curCh];
        prevR = (curCh == 2) ? in[(inFrames - 1) * 2 + 1] : prevL;
        return outFrames * 4;
    }

private:
    uint32_t curRate = 0;
    uint8_t curCh = 0;
    bool passthrough = true;
    uint64_t step = 1u << 16;
    uint64_t pos = 0;
    int16_t prevL = 0, prevR = 0;
};
