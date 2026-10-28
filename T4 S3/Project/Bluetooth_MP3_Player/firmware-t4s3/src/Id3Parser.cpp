#include "Id3Parser.h"

static const uint32_t kMaxTagScan = 512 * 1024;   // don't chase huge art frames

uint32_t Id3Parser::syncSafe(const uint8_t *p) {
    return ((uint32_t)(p[0] & 0x7F) << 21) | ((uint32_t)(p[1] & 0x7F) << 14) |
           ((uint32_t)(p[2] & 0x7F) << 7) | (uint32_t)(p[3] & 0x7F);
}

String Id3Parser::utf16ToUtf8(const uint8_t *data, size_t len, bool bigEndian) {
    String out;
    out.reserve(len / 2 + 4);
    for (size_t i = 0; i + 1 < len; i += 2) {
        uint16_t u = bigEndian ? ((uint16_t)data[i] << 8) | data[i + 1]
                               : ((uint16_t)data[i + 1] << 8) | data[i];
        if (u == 0) break;
        if (u < 0x80) {
            out += (char)u;
        } else if (u < 0x800) {
            out += (char)(0xC0 | (u >> 6));
            out += (char)(0x80 | (u & 0x3F));
        } else {
            out += (char)(0xE0 | (u >> 12));
            out += (char)(0x80 | ((u >> 6) & 0x3F));
            out += (char)(0x80 | (u & 0x3F));
        }
    }
    return out;
}

String Id3Parser::decodeTextFrame(const uint8_t *data, size_t len) {
    if (len == 0) return String();
    uint8_t enc = data[0];
    const uint8_t *body = data + 1;
    size_t bodyLen = len - 1;
    switch (enc) {
        case 0: {  // ISO-8859-1
            String s;
            s.reserve(bodyLen);
            for (size_t i = 0; i < bodyLen && body[i]; i++) s += (char)body[i];
            return s;
        }
        case 1: {  // UTF-16 with BOM
            if (bodyLen < 2) return String();
            bool be = (body[0] == 0xFE);
            return utf16ToUtf8(body + 2, bodyLen - 2, be);
        }
        case 2:  // UTF-16BE, no BOM
            return utf16ToUtf8(body, bodyLen, true);
        case 3: {  // UTF-8
            String s;
            s.reserve(bodyLen);
            for (size_t i = 0; i < bodyLen && body[i]; i++) s += (char)body[i];
            return s;
        }
        default:
            return String();
    }
}

bool Id3Parser::parseId3v2(File &f, Mp3Meta &out) {
    uint8_t hdr[10];
    if (f.read(hdr, 10) != 10) return false;
    if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3') return false;
    uint8_t ver = hdr[3];
    uint32_t tagSize = syncSafe(hdr + 6);
    if (tagSize > 8 * 1024 * 1024) tagSize = kMaxTagScan;

    uint32_t pos = 10;
    bool wantTitle = true, wantArtist = true, wantAlbum = true;
    uint8_t buf[64];

    while (pos + 10 <= tagSize && (wantTitle || wantArtist || wantAlbum)) {
        if (f.seek(pos) && f.read(buf, 10) == 10) {
            bool syncSafeSize = (ver >= 4);
            uint32_t frameSize = syncSafeSize ? syncSafe(buf + 4)
                                              : ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) |
                                                ((uint32_t)buf[6] << 8) | buf[7];
            if (frameSize == 0 || frameSize > tagSize) break;
            char id[5] = {buf[0], buf[1], buf[2], buf[3], 0};

            // Only pull the header of the frame, then read the text payload
            // (max 2 KB) - picture frames are skipped by seeking over them.
            uint32_t readLen = frameSize > 2048 ? 2048 : frameSize;
            static uint8_t frameBuf[2048];
            uint32_t got = f.read(frameBuf, readLen);
            if (got > 0) {
                String text = decodeTextFrame(frameBuf, got);
                if (!text.isEmpty()) {
                    if (wantTitle && !strcmp(id, "TIT2")) { out.title = text; wantTitle = false; }
                    else if (wantArtist && !strcmp(id, "TPE1")) { out.artist = text; wantArtist = false; }
                    else if (wantAlbum && !strcmp(id, "TALB")) { out.album = text; wantAlbum = false; }
                }
            }
            pos += 10 + frameSize;
        } else {
            break;
        }
    }
    out.audioStart = 10 + tagSize;
    return true;
}

void Id3Parser::parseId3v1(File &f, Mp3Meta &out) {
    if (f.size() < 128) return;
    uint8_t buf[128];
    if (!f.seek(f.size() - 128) || f.read(buf, 128) != 128) return;
    if (memcmp(buf, "TAG", 3) != 0) return;
    if (out.title.isEmpty()) out.title = String((const char *)buf + 3, strnlen((const char *)buf + 3, 30));
    if (out.artist.isEmpty()) out.artist = String((const char *)buf + 33, strnlen((const char *)buf + 33, 30));
}

bool Id3Parser::parseFirstFrame(File &f, Mp3Meta &out, uint32_t fileSize) {
    if (!f.seek(out.audioStart)) return false;
    uint8_t buf[64];
    if (f.read(buf, 64) != 64) return false;

    // --- MPEG frame header ---
    if (buf[0] != 0xFF || (buf[1] & 0xE0) != 0xE0) return false;
    uint8_t versionBits = (buf[1] >> 3) & 0x03;  // 0=MPEG2.5 1=res 2=MPEG2 3=MPEG1
    uint8_t layerBits = (buf[1] >> 1) & 0x03;    // 1=Layer3
    if (layerBits != 0x01) return false;         // we only care about Layer III
    bool mpeg1 = (versionBits == 0x03);

    static const uint16_t kBitrateV1L3[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
    static const uint16_t kBitrateV2L3[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
    static const uint16_t kSamplerate[4] = {44100, 48000, 32000, 0};  // MPEG1
    uint8_t brIdx = (buf[2] >> 4) & 0x0F;
    uint8_t srIdx = (buf[2] >> 2) & 0x03;
    uint8_t chMode = (buf[3] >> 6) & 0x03;

    uint32_t samplerate = kSamplerate[srIdx];
    if (!samplerate) return false;
    if (!mpeg1) samplerate = (versionBits == 0) ? samplerate / 2 : samplerate;  // MPEG2 / 2.5
    uint16_t kbps = mpeg1 ? kBitrateV1L3[brIdx] : kBitrateV2L3[brIdx];
    if (!kbps) return false;

    uint16_t samplesPerFrame = mpeg1 ? 1152 : 576;

    // --- Xing / Info header (accurate VBR duration) ---
    uint16_t sideInfo = mpeg1 ? ((chMode == 3) ? 17 : 32) : ((chMode == 3) ? 9 : 17);
    uint32_t xingOff = 4 + sideInfo;
    if (f.seek(out.audioStart + xingOff) && f.read(buf, 16) == 16) {
        if (!memcmp(buf, "Xing", 4) || !memcmp(buf, "Info", 4)) {
            uint32_t flags = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) |
                             ((uint32_t)buf[6] << 8) | buf[7];
            if (flags & 0x01) {  // frames field present
                uint32_t frames = ((uint32_t)buf[8] << 24) | ((uint32_t)buf[9] << 16) |
                                  ((uint32_t)buf[10] << 8) | buf[11];
                out.durationMs = (uint32_t)((uint64_t)frames * samplesPerFrame * 1000ULL / samplerate);
                return true;
            }
        }
    }
    // --- CBR estimate ---
    out.durationMs = (uint32_t)((uint64_t)(fileSize - out.audioStart) * 8ULL / kbps);
    return true;
}

bool Id3Parser::parse(const String &path, Mp3Meta &out) {
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    uint32_t fileSize = f.size();

    parseId3v2(f, out);
    parseFirstFrame(f, out, fileSize);
    if (out.title.isEmpty() || out.artist.isEmpty()) parseId3v1(f, out);

    f.close();
    if (out.audioStart >= fileSize) out.audioStart = 0;
    if (out.durationMs > 3600UL * 1000UL) out.durationMs = 0;
    return true;
}
