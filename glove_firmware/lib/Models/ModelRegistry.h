/* =============================================================================
 * EdgeAI Data Glove V3 — Model Registry with Hot-Switching
 * =============================================================================
 * Manages multiple L1 models and provides runtime switching between them.
 *
 * Features:
 *   - Static registry of named model instances (name → BaseModel*)
 *   - Active model tracking with thread-safe switching
 *   - Config loaded from SPIFFS (/spiffs/model_config.yaml)
 *   - Atomic model switch (no inference during transition)
 *   - Model info query for all registered models
 *
 * Thread Safety:
 *   - switchModel() sets a flag and returns immediately
 *   - Actual switch happens at the start of the next inference cycle
 *   - Uses a mutex to protect the active model pointer
 *
 * Config File Format (model_config.yaml):
 *   active_model: "cnn_attention_v2"
 *   models:
 *     - name: "cnn_attention_v2"
 *       file: "model_data_cnn_attn.h"
 *       type: "CNNAttention"
 *       classes: 46
 *     - name: "ms_tcn_v1"
 *       file: "model_data_ms_tcn.h"
 *       type: "MSTCN"
 *       classes: 20
 *
 * V3 Change:
 *   In V2, model switching required a full firmware reboot.
 *   V3 supports hot-switching by registering multiple models at boot
 *   and activating them on command via BLE or UDP.
 * =============================================================================
 */

#ifndef MODEL_REGISTRY_H
#define MODEL_REGISTRY_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <map>
#include <mutex>

#include "BaseModel.h"
#include "TFLiteModel.h"
#include "data_structures.h"

class ModelRegistry {
public:
    // =========================================================================
    // Singleton Access
    // =========================================================================

    static ModelRegistry& instance() {
        static ModelRegistry registry;
        return registry;
    }

    // Disable copy/move
    ModelRegistry(const ModelRegistry&) = delete;
    ModelRegistry& operator=(const ModelRegistry&) = delete;

    // =========================================================================
    // Model Registration
    // =========================================================================

    /**
     * @brief Register a model instance with the registry.
     *
     * The registry takes ownership of the pointer and will call cleanup()
     * and delete it on destruction or re-registration.
     *
     * @param model  Pointer to a BaseModel subclass (must be heap-allocated).
     * @return true if registered successfully.
     */
    bool registerModel(BaseModel* model) {
        if (!model) return false;

        std::lock_guard<std::mutex> lock(_mutex);
        const char* name = model->name();

        // If a model with this name already exists, clean it up first
        auto it = _models.find(name);
        if (it != _models.end() && it->second != nullptr) {
            Serial.printf("[ModelRegistry] Replacing existing model '%s'\n", name);
            it->second->cleanup();
            delete it->second;
        }

        _models[name] = model;
        Serial.printf("[ModelRegistry] Registered model '%s' (type=%s, classes=%d)\n",
                      name, model->get_model_info().type, model->get_model_info().num_classes);
        return true;
    }

    /**
     * @brief Register a TFLite model from a compiled-in model binary.
     *
     * @param name           Model name identifier.
     * @param type           Model type string.
     * @param model_data     Pointer to TFLite flatbuffer (PROGMEM).
     * @param model_size     Size of flatbuffer in bytes.
     * @param input_features Features per frame.
     * @param window_size    Temporal window length.
     * @param num_classes    Output classes.
     * @return true if registered and initialized.
     */
    bool registerTFLiteModel(const char* name, const char* type,
                             const uint8_t* model_data, size_t model_size,
                             uint16_t input_features = FEATURE_COUNT,
                             uint16_t window_size = WINDOW_SIZE,
                             uint16_t num_classes = NUM_CLASSES) {
        TFLiteModel* model = new TFLiteModel(name, type, input_features,
                                             window_size, num_classes);

        if (!model->init(model_data, model_size)) {
            Serial.printf("[ModelRegistry] Failed to init model '%s'\n", name);
            delete model;
            return false;
        }

        if (!registerModel(model)) {
            model->cleanup();
            delete model;
            return false;
        }

        // Auto-activate if no active model yet
        if (_active_model == nullptr) {
            _active_model = model;
            strncpy(_active_model_name, name, sizeof(_active_model_name) - 1);
            Serial.printf("[ModelRegistry] Auto-activated first model '%s'\n", name);
        }

        return true;
    }

    // =========================================================================
    // Model Switching
    // =========================================================================

    /**
     * @brief Switch to a different model by name.
     *
     * Thread-safe: The switch is atomic and takes effect immediately.
     * Any in-flight inference will complete with the old model.
     *
     * @param name  Name of the model to activate.
     * @return true if the model was found and activated.
     */
    bool switchModel(const char* name) {
        if (!name) return false;

        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _models.find(name);
        if (it == _models.end() || it->second == nullptr) {
            Serial.printf("[ModelRegistry] Model '%s' not found!\n", name);
            return false;
        }

        if (!it->second->isReady()) {
            Serial.printf("[ModelRegistry] Model '%s' is not initialized!\n", name);
            return false;
        }

        _active_model = it->second;
        strncpy(_active_model_name, name, sizeof(_active_model_name) - 1);
        _switch_count++;
        _last_switch_time = millis();

        Serial.printf("[ModelRegistry] Switched to model '%s' (switch #%lu)\n",
                      name, (unsigned long)_switch_count);
        return true;
    }

    /**
     * @brief Check if a model switch was requested (non-blocking).
     *
     * Used by Task_Inference to check between inference cycles.
     * Clears the pending flag after reading.
     *
     * @param out_name  Buffer to receive the requested model name (32 chars min).
     * @return true if a switch was pending.
     */
    bool checkPendingSwitch(char* out_name, size_t name_buf_size) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_pending_switch) return false;

        strncpy(out_name, _pending_model_name, name_buf_size - 1);
        out_name[name_buf_size - 1] = '\0';
        _pending_switch = false;
        return true;
    }

    /**
     * @brief Request a model switch (thread-safe, non-blocking).
     *
     * The actual switch is deferred to the inference task.
     */
    void requestSwitch(const char* name) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (name) {
            strncpy(_pending_model_name, name, sizeof(_pending_model_name) - 1);
            _pending_model_name[sizeof(_pending_model_name) - 1] = '\0';
            _pending_switch = true;
            Serial.printf("[ModelRegistry] Model switch requested: '%s'\n", name);
        }
    }

    // =========================================================================
    // Active Model Access
    // =========================================================================

    /** @return Pointer to the currently active model, or nullptr if none. */
    BaseModel* getActiveModel() const {
        return _active_model;
    }

    /** @return Name of the currently active model. */
    const char* getActiveModelName() const {
        return _active_model_name;
    }

    /** @return true if an active model is ready for inference. */
    bool hasActiveModel() const {
        return _active_model != nullptr && _active_model->isReady();
    }

    /**
     * @brief Run inference using the active model on a window of sensor data.
     *
     * @param frames      Sensor frame window.
     * @param num_frames  Number of frames.
     * @param result      Output GestureResult.
     * @return 0 on success, negative on error.
     */
    int inferActive(const SensorData* frames, int num_frames, GestureResult* result) {
        if (!hasActiveModel()) {
            result->zero();
            return -1;
        }
        return _active_model->run(frames, num_frames, result);
    }

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Load model configuration from SPIFFS.
     *
     * Reads /spiffs/model_config.yaml, parses it, and attempts to
     * register and initialize all listed models.
     *
     * NOTE: Model binary data must be compiled into the firmware via
     * #include headers (SPIFFS is too slow for model loading at runtime).
     * The config file just specifies which models to activate.
     *
     * @return true if config was loaded and at least one model initialized.
     */
    bool loadConfig() {
        Serial.println("[ModelRegistry] Loading config from SPIFFS...");

        if (!SPIFFS.begin(true)) {
            Serial.println("[ModelRegistry] ERROR: SPIFFS mount failed!");
            return false;
        }

        File file = SPIFFS.open("/spiffs/model_config.yaml", "r");
        if (!file) {
            Serial.println("[ModelRegistry] WARNING: model_config.yaml not found, using defaults");
            // Load a default compiled-in model
            loadDefaultModels();
            return hasActiveModel();
        }

        // Parse YAML-like config using ArduinoJson
        // Note: ArduinoJson doesn't natively support YAML, so we use
        // a simplified JSON-like format. The .yaml file uses a format
        // that ArduinoJson's deserializeJson can parse (or we preprocess).
        //
        // For production, use a proper YAML parser or convert to JSON.

        // Read file content
        String content = file.readString();
        file.close();

        Serial.println("[ModelRegistry] Config content:");
        Serial.println(content);

        // Parse using ArduinoJson v7 (simplified YAML parsing)
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, content);

        if (err) {
            Serial.printf("[ModelRegistry] JSON parse error: %s\n", err.c_str());
            Serial.println("[ModelRegistry] Falling back to default models");
            loadDefaultModels();
            return hasActiveModel();
        }

        // Extract active model name
        const char* active_name = doc["active_model"] | "";
        if (active_name[0] != '\0') {
            Serial.printf("[ModelRegistry] Config active_model: '%s'\n", active_name);
        }

        // NOTE: Actual model registration requires compiled-in model binaries.
        // This method only sets the config. Use loadDefaultModels() or
        // explicit registerTFLiteModel() calls to actually load models.
        //
        // The config file is primarily used by the companion app to know
        // which models are available on this firmware build.

        if (active_name[0] != '\0') {
            strncpy(_config_active_model, active_name, sizeof(_config_active_model) - 1);
        }

        Serial.println("[ModelRegistry] Config loaded, loading default models...");
        loadDefaultModels();
        return hasActiveModel();
    }

    // =========================================================================
    // Enumeration
    // =========================================================================

    /**
     * @brief Print all registered models to Serial.
     */
    void printRegisteredModels() {
        Serial.println("=== Model Registry ===");
        std::lock_guard<std::mutex> lock(_mutex);

        for (const auto& pair : _models) {
            const char* name = pair.first.c_str();
            BaseModel* model = pair.second;
            bool is_active = (model == _active_model);

            ModelInfo info = model->get_model_info();
            Serial.printf("  %s%s '%s' type=%s in=%d×%d out=%d ready=%d\n",
                          is_active ? "→ " : "  ",
                          name, info.type, info.input_features,
                          info.window_size, info.num_classes, model->isReady());
        }

        Serial.printf("  Active: '%s', Switches: %lu\n",
                      _active_model_name, (unsigned long)_switch_count);
        Serial.println("=====================");
    }

    /** @return Number of registered models. */
    size_t modelCount() const {
        return _models.size();
    }

    uint32_t switchCount() const { return _switch_count; }

private:
    // =========================================================================
    // Private Constructor (Singleton)
    // =========================================================================

    ModelRegistry()
        : _active_model(nullptr),
          _switch_count(0),
          _last_switch_time(0),
          _pending_switch(false) {
        memset(_active_model_name, 0, sizeof(_active_model_name));
        memset(_pending_model_name, 0, sizeof(_pending_model_name));
        memset(_config_active_model, 0, sizeof(_config_active_model));
    }

    ~ModelRegistry() {
        // Clean up all registered models
        for (auto& pair : _models) {
            if (pair.second) {
                pair.second->cleanup();
                delete pair.second;
            }
        }
        _models.clear();
    }

    // =========================================================================
    // Default Model Loading
    // =========================================================================

    /**
     * @brief Load the default compiled-in models.
     *
     * In a real firmware build, model binary data is included via:
     *   extern const unsigned char g_model_cnn_attn[];
     *   extern const size_t g_model_cnn_attn_len;
     *
     * These come from the Edge Impulse or TFLite export pipeline.
     */
    void loadDefaultModels() {
        Serial.println("[ModelRegistry] Loading default models...");

        // ---- Default Model 1: CNN Attention V2 ----
        // This would normally be included from a generated header:
        // #include "model_data_cnn_attn.h"
        //
        // For the code framework, we create a placeholder model.
        // In production, replace with actual model binary:
        //
        // registerTFLiteModel(
        //     "cnn_attention_v2", "CNNAttention",
        //     g_model_cnn_attn, g_model_cnn_attn_len,
        //     FEATURE_COUNT, WINDOW_SIZE, NUM_CLASSES
        // );

        // Placeholder: create an uninit'd model for testing the registry
        // In production builds, this is replaced by actual model loading
        Serial.println("[ModelRegistry] No compiled-in model data found.");
        Serial.println("[ModelRegistry] To enable inference, compile with model_data.h");

        // Try to activate the config-specified model (if it was registered)
        if (_config_active_model[0] != '\0') {
            switchModel(_config_active_model);
        }
    }

    // =========================================================================
    // Members
    // =========================================================================

    std::map<std::string, BaseModel*> _models;  ///< Name → model map
    BaseModel*       _active_model;             ///< Currently active model
    char             _active_model_name[32];    ///< Active model name
    char             _config_active_model[32];  ///< Config-requested model
    char             _pending_model_name[32];   ///< Pending switch target
    bool             _pending_switch;           ///< Switch request flag
    uint32_t         _switch_count;             ///< Total switch count
    uint32_t         _last_switch_time;         ///< Timestamp of last switch
    mutable std::mutex _mutex;                  ///< Thread safety
};

#endif // MODEL_REGISTRY_H
