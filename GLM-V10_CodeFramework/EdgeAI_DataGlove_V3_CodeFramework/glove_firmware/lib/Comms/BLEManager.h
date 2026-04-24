/* =============================================================================
 * EdgeAI Data Glove V3 — BLE 5.0 Provisioning Manager (NimBLE)
 * =============================================================================
 * Provides BLE GATT services for:
 *   1. WiFi Credential Provisioning — Central (phone) writes SSID/password
 *   2. Low-Speed Data Notification — Fallback telemetry at ~20 Hz when WiFi
 *      is unavailable (e.g., during provisioning or WiFi failure).
 *
 * BLE Stack: NimBLE-Arduino (replaces Bluedroid — lower RAM, better concurrency)
 *
 * GATT Service UUID: 0x181A (Environmental Sensing) — reuse for custom data
 * Characteristic UUIDs:
 *   - 0x2A6E → WiFi SSID (write-only, 32 bytes max)
 *   - 0x2A6F → WiFi Password (write-only, 64 bytes max)
 *   - 0x2A58 → Provisioning Status (notify, read)
 *   - 0x2A19 → Battery Level (read, notify) — reserved
 *   - Custom UUID → Sensor Data (notify, 20 Hz, encoded as CSV)
 *
 * Thread Safety:
 *   NimBLE callbacks run on the NimBLE task (not our tasks).
 *   Use _ssid_ready flag + semaphore for safe cross-task signaling.
 *
 * V3 Note: In V2, Bluedroid BLE caused watchdog resets when combined with
 *   100Hz sensor reads. NimBLE resolves this by using a dedicated task.
 * =============================================================================
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "data_structures.h"

// Forward declaration for optional NanoPB integration
// #include "glove_data.pb.h"

class BLEManager {
public:
    // =========================================================================
    // BLE UUIDs (custom 128-bit for data service)
    // =========================================================================

    /// Service UUID: Custom sensor data service
    static const NimBLEUUID kServiceUUID;

    /// WiFi SSID characteristic (write)
    static const NimBLEUUID kSSIDCharUUID;

    /// WiFi Password characteristic (write)
    static const NimBLEUUID kPassCharUUID;

    /// Provisioning status characteristic (notify + read)
    static const NimBLEUUID kStatusCharUUID;

    /// Sensor data notification characteristic (notify, 20 Hz)
    static const NimBLEUUID kDataCharUUID;

    // =========================================================================
    // Constructor
    // =========================================================================

    BLEManager()
        : _provisioned(false),
          _ssid_ready(false),
          _pass_ready(false),
          _server(nullptr),
          _data_char(nullptr),
          _status_char(nullptr),
          _notify_enabled(false),
          _last_notify_ms(0) {
        _ssid[0] = '\0';
        _pass[0] = '\0';
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Initialize NimBLE device, server, and GATT services.
     *
     * Device name: "DataGlove-V3"
     * Advertises custom service UUID + WiFi provisioning capability.
     *
     * @param device_name  BLE advertising name (default "DataGlove-V3").
     * @return true if BLE stack started successfully.
     */
    bool begin(const char* device_name = "DataGlove-V3") {
        Serial.println("[BLE] Initializing NimBLE...");

        // Initialize NimBLE device
        NimBLEDevice::init(device_name);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // 9 dBm (max)

        // Set MTU for larger data payloads (default is 23, we need more for sensor data)
        NimBLEDevice::setMTU(512);

        // Create server
        _server = NimBLEDevice::createServer();
        _server->setCallbacks(new ServerCallbacksImpl(this));

        // Create service
        NimBLEService* service = _server->createService(kServiceUUID);

        // ---- WiFi SSID Characteristic (write-only) ----
        NimBLECharacteristic* ssid_char = service->createCharacteristic(
            kSSIDCharUUID,
            NIMBLE_PROPERTY::WRITE_NR
        );
        ssid_char->setCallbacks(new SSIDCallbackImpl(this));
        ssid_char->setValue("");

        // ---- WiFi Password Characteristic (write-only) ----
        NimBLECharacteristic* pass_char = service->createCharacteristic(
            kPassCharUUID,
            NIMBLE_PROPERTY::WRITE_NR
        );
        pass_char->setCallbacks(new PassCallbackImpl(this));
        pass_char->setValue("");

        // ---- Provisioning Status Characteristic (notify + read) ----
        _status_char = service->createCharacteristic(
            kStatusCharUUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        _status_char->setValue("PROVISIONING");

        // ---- Sensor Data Characteristic (notify, 20 Hz) ----
        _data_char = service->createCharacteristic(
            kDataCharUUID,
            NIMBLE_PROPERTY::NOTIFY
        );

        // Start service
        service->start();

        // Set up advertising
        NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
        advertising->addServiceUUID(kServiceUUID);
        advertising->setScanResponse(true);
        advertising->setMinPreferred(0x06);  // Help with iPhone connections
        advertising->setMinPreferred(0x12);  // More help
        NimBLEDevice::startAdvertising();

        Serial.println("[BLE] NimBLE advertising as 'DataGlove-V3'");
        return true;
    }

    // =========================================================================
    // WiFi Credential Access
    // =========================================================================

    /**
     * @brief Check if WiFi credentials have been provisioned.
     * @return true if both SSID and password have been received via BLE.
     */
    bool isProvisioned() const {
        return _ssid_ready && _pass_ready &&
               (_ssid[0] != '\0') && (_pass[0] != '\0');
    }

    /** @return Provisioned WiFi SSID (null-terminated). */
    const char* provisionedSSID() const { return _ssid; }

    /** @return Provisioned WiFi password (null-terminated). */
    const char* provisionedPASS() const { return _pass; }

    /**
     * @brief Clear stored credentials (for re-provisioning).
     */
    void clearCredentials() {
        _ssid[0] = '\0';
        _pass[0] = '\0';
        _ssid_ready = false;
        _pass_ready = false;
        _provisioned = false;
        if (_status_char) {
            _status_char->setValue("PROVISIONING");
            _status_char->notify();
        }
        Serial.println("[BLE] Credentials cleared, ready for re-provisioning");
    }

    // =========================================================================
    // Data Notification
    // =========================================================================

    /**
     * @brief Send sensor data notification over BLE.
     *
     * Rate-limited to 20 Hz (50 ms interval) to respect BLE bandwidth.
     * Data is encoded as a compact CSV string:
     *   "t=<ts>,h=<15 floats>,i=<6 floats>,g=<id>,c=<conf>"
     *
     * @param packet  FullDataPacket to transmit.
     */
    void notifyData(const FullDataPacket& packet) {
        if (!_data_char || !_notify_enabled) return;

        // Rate limit to 20 Hz
        uint32_t now = millis();
        if (now - _last_notify_ms < 50) return;
        _last_notify_ms = now;

        // Encode as compact CSV (lighter than protobuf for BLE)
        char buf[256];
        int len = snprintf(buf, sizeof(buf),
            "t=%lu,"
            "h=%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,"
            "i=%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,"
            "g=%d,c=%.2f",
            (unsigned long)packet.sensor.timestamp_us,
            packet.sensor.hall_xyz[0],  packet.sensor.hall_xyz[1],  packet.sensor.hall_xyz[2],
            packet.sensor.hall_xyz[3],  packet.sensor.hall_xyz[4],  packet.sensor.hall_xyz[5],
            packet.sensor.hall_xyz[6],  packet.sensor.hall_xyz[7],  packet.sensor.hall_xyz[8],
            packet.sensor.hall_xyz[9],  packet.sensor.hall_xyz[10], packet.sensor.hall_xyz[11],
            packet.sensor.hall_xyz[12], packet.sensor.hall_xyz[13], packet.sensor.hall_xyz[14],
            packet.sensor.euler[0], packet.sensor.euler[1], packet.sensor.euler[2],
            packet.sensor.gyro[0],  packet.sensor.gyro[1],  packet.sensor.gyro[2],
            packet.inference.gesture_id,
            packet.inference.confidence);

        if (len > 0 && len < (int)sizeof(buf)) {
            _data_char->setValue((uint8_t*)buf, len);
            _data_char->notify();
        }
    }

    // =========================================================================
    // Status
    // =========================================================================

    /**
     * @brief Update the provisioning status characteristic.
     * @param status  Status string (e.g., "CONNECTED", "PROVISIONING", "STREAMING").
     */
    void setStatus(const char* status) {
        if (_status_char) {
            _status_char->setValue(status);
            _status_char->notify();
        }
    }

private:
    // =========================================================================
    // State
    // =========================================================================

    char  _ssid[33];           ///< WiFi SSID (max 32 chars + null)
    char  _pass[65];           ///< WiFi password (max 64 chars + null)
    bool  _provisioned;
    bool  _ssid_ready;
    bool  _pass_ready;

    NimBLEServer*              _server;
    NimBLECharacteristic*      _data_char;
    NimBLECharacteristic*      _status_char;
    bool                       _notify_enabled;
    uint32_t                   _last_notify_ms;

    // =========================================================================
    // NimBLE Callbacks (nested classes)
    // =========================================================================

    /** Server connection/disconnection callbacks */
    class ServerCallbacksImpl : public NimBLEServerCallbacks {
    public:
        explicit ServerCallbacksImpl(BLEManager* mgr) : _mgr(mgr) {}

        void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
            _mgr->_notify_enabled = true;
            _mgr->setStatus("CONNECTED");
            Serial.printf("[BLE] Client connected (handle=%d)\n", connInfo.getConnHandle());
        }

        void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
            _mgr->_notify_enabled = false;
            _mgr->setStatus("DISCONNECTED");
            Serial.printf("[BLE] Client disconnected (reason=%d)\n", reason);
            // Restart advertising after disconnect
            NimBLEDevice::startAdvertising();
        }
    private:
        BLEManager* _mgr;
    };

    /** WiFi SSID write callback */
    class SSIDCallbackImpl : public NimBLECharacteristicCallbacks {
    public:
        explicit SSIDCallbackImpl(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            std::string val = pChar->getValue();
            if (val.length() > 0 && val.length() < sizeof(_mgr->_ssid)) {
                strncpy(_mgr->_ssid, val.c_str(), sizeof(_mgr->_ssid) - 1);
                _mgr->_ssid[sizeof(_mgr->_ssid) - 1] = '\0';
                _mgr->_ssid_ready = true;
                Serial.printf("[BLE] SSID received: '%s'\n", _mgr->_ssid);

                // Check if both SSID and password are ready
                if (_mgr->isProvisioned()) {
                    _mgr->_provisioned = true;
                    _mgr->setStatus("PROVISIONED");
                    Serial.println("[BLE] WiFi credentials complete!");
                }
            }
        }
    private:
        BLEManager* _mgr;
    };

    /** WiFi Password write callback */
    class PassCallbackImpl : public NimBLECharacteristicCallbacks {
    public:
        explicit PassCallbackImpl(BLEManager* mgr) : _mgr(mgr) {}

        void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
            std::string val = pChar->getValue();
            if (val.length() > 0 && val.length() < sizeof(_mgr->_pass)) {
                strncpy(_mgr->_pass, val.c_str(), sizeof(_mgr->_pass) - 1);
                _mgr->_pass[sizeof(_mgr->_pass) - 1] = '\0';
                _mgr->_pass_ready = true;
                Serial.printf("[BLE] Password received: (%zu chars)\n", val.length());

                // Check if both SSID and password are ready
                if (_mgr->isProvisioned()) {
                    _mgr->_provisioned = true;
                    _mgr->setStatus("PROVISIONED");
                    Serial.println("[BLE] WiFi credentials complete!");
                }
            }
        }
    private:
        BLEManager* _mgr;
    }
};

// =============================================================================
// Static UUID definitions (declared above as static const)
// =============================================================================

// Custom 128-bit UUIDs for our service and characteristics
// Generated with: uuidgen or online UUID v4 generator
const NimBLEUUID BLEManager::kServiceUUID  = NimBLEUUID("0000181a-0000-1000-8000-00805f9b34fb");  // Environmental Sensing
const NimBLEUUID BLEManager::kSSIDCharUUID = NimBLEUUID("00002a6e-0000-1000-8000-00805f9b34fb");
const NimBLEUUID BLEManager::kPassCharUUID = NimBLEUUID("00002a6f-0000-1000-8000-00805f9b34fb");
const NimBLEUUID BLEManager::kStatusCharUUID = NimBLEUUID("00002a58-0000-1000-8000-00805f9b34fb");
const NimBLEUUID BLEManager::kDataCharUUID  = NimBLEUUID("a1b2c3d4-e5f6-7890-abcd-123456789abc");  // Custom data UUID

#endif // BLE_MANAGER_H
