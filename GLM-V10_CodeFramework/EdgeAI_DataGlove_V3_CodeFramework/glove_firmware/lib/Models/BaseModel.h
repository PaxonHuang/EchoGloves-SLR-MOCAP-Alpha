/* =============================================================================
 * EdgeAI Data Glove V3 — Abstract Model Interface (BaseModel)
 * =============================================================================
 * Polymorphic interface for all on-device inference models (L1).
 * Enables runtime model hot-switching via the ModelRegistry.
 *
 * Design Principles:
 *   1. Pure virtual interface — all implementations must provide init/preprocess/infer/postprocess
 *   2. Zero-copy where possible — input/output buffers are caller-owned
 *   3. PSRAM-friendly — arena allocation happens in init(), not on the stack
 *   4. Thread-safe — implementations must handle concurrent infer() calls
 *      (typically by using a mutex or running on a dedicated task)
 *
 * Inference Pipeline:
 *   SensorFrame[] → preprocess() → float[] → infer() → float[] → postprocess() → GestureResult
 *
 * Implementations:
 *   - TFLiteModel (lib/Models/TFLiteModel.h) — TensorFlow Lite for Microcontrollers
 *   - (Future) EdgeTPUModel — for Coral TPU accelerator
 *   - (Future) ONNXModel — ONNX Runtime Micro
 * =============================================================================
 */

#ifndef BASE_MODEL_H
#define BASE_MODEL_H

#include <cstdint>
#include <cstddef>
#include "data_structures.h"

// =============================================================================
// GestureResult — Extended with per-class scores and metadata
// =============================================================================

// (Already defined in data_structures.h, re-using that definition)

// =============================================================================
// ModelInfo — Runtime model metadata
// =============================================================================

// (Already defined in data_structures.h, re-using that definition)

// =============================================================================
// BaseModel — Abstract Interface
// =============================================================================

class BaseModel {
public:
    virtual ~BaseModel() = default;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    /**
     * @brief Initialize the model with binary model data.
     *
     * The model_data pointer must remain valid for the lifetime of the model
     * (typically a PROGMEM const array compiled into flash).
     *
     * Implementations should:
     *   1. Parse model flatbuffer / tflite header
     *   2. Allocate tensor arena in PSRAM (ps_malloc)
     *   3. Build the interpreter
     *   4. Verify input/output tensor shapes match expected dimensions
     *
     * @param model_data  Pointer to model binary (in flash/PSRAM).
     * @param model_size  Size of model binary in bytes.
     * @return true if initialization succeeded.
     */
    virtual bool init(const uint8_t* model_data, size_t model_size) = 0;

    /**
     * @brief Release all allocated resources (arena, interpreter, etc.)
     *
     * Called before hot-switching to a different model or on shutdown.
     * After cleanup(), init() must be called again before infer().
     */
    virtual void cleanup() = 0;

    // =========================================================================
    // Inference Pipeline
    // =========================================================================

    /**
     * @brief Preprocess sensor frames into a flat float array for the model.
     *
     * Typical preprocessing:
     *   1. Normalize each feature to zero mean, unit variance (using training stats)
     *   2. For temporal models: arrange frames into [window × features] matrix
     *   3. For single-frame models: use only the latest frame
     *
     * @param input       Output buffer for preprocessed features.
     *                    Size must be ≥ (window_size × input_features).
     * @param frames      Array of sensor frames (size = num_frames).
     * @param num_frames  Number of frames in the window.
     * @return Number of float values written to input, or -1 on error.
     */
    virtual int preprocess(float* input, const SensorData* frames, int num_frames) = 0;

    /**
     * @brief Run model inference on preprocessed input.
     *
     * @param input   Preprocessed feature array (from preprocess()).
     * @param output  Output buffer for raw model logits.
     *                Size must be ≥ num_classes.
     * @return Number of output values written, or -1 on error.
     */
    virtual int infer(const float* input, float* output) = 0;

    /**
     * @brief Post-process raw logits into a GestureResult.
     *
     * Typical postprocessing:
     *   1. Apply softmax to get probabilities
     *   2. Find argmax (top-1 prediction)
     *   3. Check if confidence exceeds threshold
     *   4. Set l2_requested if confidence is in the "uncertain" band
     *
     * @param output  Raw model output (logits or probabilities).
     * @param result  Output GestureResult struct.
     * @return 0 on success, -1 on error.
     */
    virtual int postprocess(const float* output, GestureResult* result) = 0;

    // =========================================================================
    // Convenience: Full Pipeline
    // =========================================================================

    /**
     * @brief Run the complete inference pipeline: preprocess → infer → postprocess.
     *
     * @param frames      Sensor frame window.
     * @param num_frames  Number of frames in the window.
     * @param result      Output GestureResult.
     * @return 0 on success, negative on error.
     */
    virtual int run(const SensorData* frames, int num_frames, GestureResult* result) {
        if (!frames || !result) return -1;

        // Allocate input buffer in PSRAM for potentially large temporal models
        ModelInfo info = get_model_info();
        size_t input_size = (size_t)info.window_size * info.input_features;
        size_t output_size = info.num_classes;

        float* input_buf = (float*)ps_malloc(input_size * sizeof(float));
        float* output_buf = (float*)ps_malloc(output_size * sizeof(float));

        if (!input_buf || !output_buf) {
            Serial.println("[BaseModel] ERROR: Failed to allocate PSRAM buffers");
            if (input_buf) free(input_buf);
            if (output_buf) free(output_buf);
            result->zero();
            return -1;
        }

        int ret = preprocess(input_buf, frames, num_frames);
        if (ret < 0) {
            free(input_buf); free(output_buf);
            result->zero();
            return ret;
        }

        ret = infer(input_buf, output_buf);
        if (ret < 0) {
            free(input_buf); free(output_buf);
            result->zero();
            return ret;
        }

        ret = postprocess(output_buf, result);

        free(input_buf);
        free(output_buf);
        return ret;
    }

    // =========================================================================
    // Metadata
    // =========================================================================

    /**
     * @brief Get runtime model metadata.
     * @return ModelInfo struct with name, dimensions, class count, etc.
     */
    virtual ModelInfo get_model_info() const = 0;

    /**
     * @brief Get model name (short alias for model_config.yaml lookup).
     * @return Null-terminated model name string.
     */
    virtual const char* name() const = 0;

    // =========================================================================
    // State Query
    // =========================================================================

    /** @return true if init() has been called successfully. */
    virtual bool isReady() const = 0;

    /** @return Inference latency in microseconds (for the last infer() call). */
    virtual uint32_t lastInferenceTimeUs() const = 0;
};

#endif // BASE_MODEL_H
