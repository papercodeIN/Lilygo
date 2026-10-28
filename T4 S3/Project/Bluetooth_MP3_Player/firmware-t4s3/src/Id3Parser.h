#pragma once

#include <Arduino.h>
#include <SD.h>

// =============================================================================
//  Minimal ID3v2.3/2.4 + ID3v1 tag reader and MP3 duration estimator.
//  Reads only the head of the file (no album-art sized reads unless the text
//  frames sit behind a big picture frame - bounded by kMaxTagScan).
// =============================================================================

struct Mp3Meta {
    String title;
    String artist;
    String album;
    uint32_t audioStart = 0;     // byte offset of first MPEG frame
    uint32_t durationMs = 0;     // 0 = unknown
};

class Id3Parser {
public:
    // Parses `path` (open on any FS: SD, SD_MMC, SPIFFS...). Returns false on
    // IO failure; tag fields simply stay empty when no tag is present.
    static bool parse(const String &path, Mp3Meta &out);

private:
    static bool parseId3v2(File &f, Mp3Meta &out);
    static void parseId3v1(File &f, Mp3Meta &out);
    static bool parseFirstFrame(File &f, Mp3Meta &out, uint32_t fileSize);
    static String decodeTextFrame(const uint8_t *data, size_t len);
    static String utf16ToUtf8(const uint8_t *data, size_t len, bool bigEndian);
    static uint32_t syncSafe(const uint8_t *p);
};
