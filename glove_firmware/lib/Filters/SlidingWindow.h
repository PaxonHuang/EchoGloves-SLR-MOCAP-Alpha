/* =============================================================================
 * EdgeAI Data Glove V3 — Sliding Window Ring Buffer
 * =============================================================================
 * Circular buffer for 30-frame (300ms @ 100Hz) sensor data windows.
 * Used as input buffer for L1 inference (TFLite Micro).
 *
 * Single-producer (Task_SensorRead) / single-consumer (Task_Inference).
 * No mutex needed — SPSC ring buffer with atomic frame counter.
 *
 * Memory: 30 × 21 × 4 bytes = 2520 bytes, allocated in PSRAM.
 * =============================================================================
 */

#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H

#include "data_structures.h"
#include <cstring>

class SlidingWindow {
public:
    SlidingWindow()
        : _write_idx(0), _frame_count(0), _full(false) {

        // Allocate contiguous buffer — prefer PSRAM for large buffers
        size_t total_bytes = WINDOW_SIZE * FEATURE_COUNT * sizeof(float);

#ifdef BOARD_HAS_PSRAM
        _buffer = (float*)heap_caps_malloc(total_bytes, MALLOC_CAP_SPIRAM);
        if (_buffer == nullptr) {
            Serial.println("[SlidingWindow] WARNING: PSRAM alloc failed, using heap");
            _buffer = (float*)malloc(total_bytes);
        }
#else
        _buffer = (float*)malloc(total_bytes);
#endif

        memset(_buffer, 0, total_bytes);

        Serial.printf("[SlidingWindow] Init: %d frames × %d features = %d bytes\n",
                      WINDOW_SIZE, FEATURE_COUNT, (int)total_bytes);
    }

    ~SlidingWindow() {
        free(_buffer);
    }

    /**
     * @brief Push a feature frame into the ring buffer.
     * Overwrites oldest frame when buffer is full.
     * @param features  Array of FEATURE_COUNT floats (normalized + Kalman-filtered).
     * @return Write index (0 to WINDOW_SIZE-1).
     */
    uint16_t push(const float* features) {
        uint16_t pos = _write_idx % WINDOW_SIZE;
        memcpy(_buffer + pos * FEATURE_COUNT, features, FEATURE_COUNT * sizeof(float));

        _write_idx++;
        if (_frame_count < WINDOW_SIZE) {
            _frame_count++;
        }
        if (_write_idx >= WINDOW_SIZE) {
            _full = true;
        }

        return pos;
    }

    /**
     * @brief Get contiguous buffer for inference input.
     * Layout: [frame0_feats, frame1_feats, ..., frame29_feats]
     * Total: WINDOW_SIZE × FEATURE_COUNT floats = 630 values.
     * @return Const pointer to buffer start.
     */
    const float* getBuffer() const { return _buffer; }

    /** @return true if WINDOW_SIZE frames have been pushed (ready for inference). */
    bool isFull() const { return _full; }

    /** @return Current frame count (0 to WINDOW_SIZE). */
    uint16_t getFrameCount() const { return _frame_count; }

    /** @brief Reset to empty state. */
    void clear() {
        _write_idx = 0;
        _frame_count = 0;
        _full = false;
        memset(_buffer, 0, WINDOW_SIZE * FEATURE_COUNT * sizeof(float));
    }

private:
    float*    _buffer;          ///< Contiguous ring buffer memory
    uint16_t  _write_idx;       ///< Next write position (wraps at WINDOW_SIZE)
    uint16_t  _frame_count;     ///< Frames currently stored (0 to WINDOW_SIZE)
    bool      _full;            ///< Has wrapped at least once
};

#endif // SLIDING_WINDOW_H