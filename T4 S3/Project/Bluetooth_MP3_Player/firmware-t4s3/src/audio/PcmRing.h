#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// =============================================================================
//  Single-producer / single-consumer byte ring buffer.
//  Producer and consumer may run in different FreeRTOS tasks; head is only
//  written by the producer, tail only by the consumer, each inside a short
//  critical section so 32-bit reads stay atomic on both cores.
// =============================================================================
class PcmRing {
public:
    bool init(size_t size) {
        buf = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!buf) buf = (uint8_t *)malloc(size);
        if (!buf) return false;
        capacity = size;
        head = tail = 0;
        return true;
    }

    size_t used() const {
        uint32_t h = head, t = tail;
        return (h >= t) ? (h - t) : (capacity - t + h);
    }

    size_t freeSpace() const { return capacity - 1 - used(); }

    // Producer: copy bytes in. Blocks in 5 ms slices while full, up to timeoutMs.
    size_t push(const uint8_t *data, size_t len, uint32_t timeoutMs = 200) {
        size_t written = 0;
        TickType_t start = xTaskGetTickCount();
        while (written < len) {
            size_t n = writeOnce(data + written, len - written);
            if (n) {
                written += n;
                continue;
            }
            if ((xTaskGetTickCount() - start) * portTICK_PERIOD_MS >= timeoutMs) break;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        return written;
    }

    // Consumer: pop up to len bytes, returns how many were actually popped.
    size_t pop(uint8_t *out, size_t len) {
        size_t got = 0;
        uint32_t h = head, t = tail;
        while (got < len && t != h) {
            size_t contig = (t < h) ? (h - t) : (capacity - t);
            if (contig > len - got) contig = len - got;
            memcpy(out + got, buf + t, contig);
            got += contig;
            t = (t + contig) % capacity;
        }
        if (got) {
            taskENTER_CRITICAL(&mux);
            tail = t;
            taskEXIT_CRITICAL(&mux);
        }
        return got;
    }

    void reset() {
        taskENTER_CRITICAL(&mux);
        head = tail = 0;
        taskEXIT_CRITICAL(&mux);
    }

private:
    size_t writeOnce(const uint8_t *data, size_t len) {
        uint32_t h = head, t = tail;
        size_t usedBytes = (h >= t) ? (h - t) : (capacity - t + h);
        size_t freeBytes = capacity - 1 - usedBytes;
        if (!freeBytes || !len) return 0;
        size_t n = (freeBytes < len) ? freeBytes : len;
        size_t contig = capacity - h;
        if (contig > n) contig = n;
        memcpy(buf + h, data, contig);
        if (n > contig) memcpy(buf, data + contig, n - contig);
        taskENTER_CRITICAL(&mux);
        head = (uint32_t)((h + n) % capacity);
        taskEXIT_CRITICAL(&mux);
        return n;
    }

    uint8_t *buf = nullptr;
    size_t capacity = 0;
    volatile uint32_t head = 0;
    volatile uint32_t tail = 0;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};
