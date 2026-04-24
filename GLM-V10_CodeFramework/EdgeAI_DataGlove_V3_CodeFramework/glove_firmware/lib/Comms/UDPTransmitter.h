/* =============================================================================
 * EdgeAI Data Glove V3 — WiFi UDP Transmitter (AsyncUDP)
 * =============================================================================
 * Sends serialized sensor + inference data over WiFi UDP at up to 100 Hz.
 *
 * Architecture:
 *   - Uses AsyncUDP (non-blocking, runs on WiFi event task)
 *   - Target: broadcast or unicast to edge inference server
 *   - Serialization: Nanopb (generated from glove_data.proto)
 *   - Fallback: If WiFi unavailable, data is silently dropped (BLE takes over)
 *
 * Packet Format:
 *   Nanopb-encoded GloveData message (~130 bytes per packet at 100 Hz = 13 KB/s)
 *
 * Thread Safety:
 *   AsyncUDP's send() is thread-safe (internally queues to the WiFi task).
 *   send() can be called from any FreeRTOS task.
 *
 * V3 Change: Added L1 inference fields to the UDP packet for edge server
 *   to decide whether L2 inference is needed.
 * =============================================================================
 */

#ifndef UDP_TRANSMITTER_H
#define UDP_TRANSMITTER_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include "data_structures.h"

// Forward declaration for nanopb generated header
// #include "glove_data.pb.h"

class UDPTransmitter {
public:
    // =========================================================================
    // Constructor
    // =========================================================================

    UDPTransmitter()
        : _udp(nullptr),
          _connected(false),
          _target_port(UDP_PORT),
          _packets_sent(0),
          _packets_dropped(0),
          _last_send_us(0) {
        memset(_target_ip, 0, sizeof(_target_ip));
        _target_ip[0] = 255; _target_ip[1] = 255;
        _target_ip[2] = 255; _target_ip[3] = 255;  // Default: broadcast
    }

    // =========================================================================
    // Destructor
    // =========================================================================

    ~UDPTransmitter() {
        if (_udp) {
            _udp->close();
            delete _udp;
            _udp = nullptr;
        }
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Connect to WiFi and start UDP transmitter.
     *
     * @param ssid        WiFi network name.
     * @param pass        WiFi password.
     * @param target_ip   Dotted-quad target IP (e.g., "192.168.1.100").
     *                    Use "255.255.255.255" for broadcast.
     * @param port        UDP port (default 8888).
     * @return true if WiFi connected and UDP ready.
     */
    bool begin(const char* ssid, const char* pass,
               const char* target_ip, uint16_t port = UDP_PORT) {
        // ---- Step 1: Connect to WiFi ----
        Serial.printf("[UDP] Connecting to WiFi '%s'...\n", ssid);

        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);       // Keep WiFi awake for low latency
        WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Maximum TX power
        WiFi.begin(ssid, pass);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 40) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();

        if (WiFi.status() != WL_CONNECTED) {
            Serial.printf("[UDP] WiFi connection FAILED (status=%d)\n", WiFi.status());
            _connected = false;
            return false;
        }

        Serial.printf("[UDP] WiFi connected: IP=%s, RSSI=%d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        Serial.printf("[UDP] Gateway: %s, DNS: %s\n",
                      WiFi.gatewayIP().toString().c_str(),
                      WiFi.dnsIP().toString().c_str());

        // ---- Step 2: Parse target IP ----
        strncpy(_target_ip, target_ip, sizeof(_target_ip) - 1);
        _target_ip[sizeof(_target_ip) - 1] = '\0';
        _target_port = port;

        // ---- Step 3: Start AsyncUDP ----
        _udp = new AsyncUDP();
        if (!_udp->listen(IPAddress(0, 0, 0, 0), 0)) {  // Listen on ephemeral port
            Serial.println("[UDP] AsyncUDP listen() failed!");
            delete _udp;
            _udp = nullptr;
            _connected = false;
            return false;
        }

        // Register packet handler (for receiving commands from server)
        _udp->onPacket([this](AsyncUDPPacket packet) {
            handleIncomingPacket(packet);
        });

        _connected = true;
        _packets_sent = 0;
        _packets_dropped = 0;

        Serial.printf("[UDP] Transmitting to %s:%d\n", _target_ip, _target_port);
        return true;
    }

    // =========================================================================
    // Transmission
    // =========================================================================

    /**
     * @brief Serialize and send a FullDataPacket over UDP.
     *
     * The packet is serialized using Nanopb (or a compact binary format
     * if nanopb is not yet integrated) and sent to the target IP:port.
     *
     * Rate: Up to 100 Hz (caller is responsible for rate limiting).
     *
     * @param packet  FullDataPacket containing sensor data + inference.
     * @return true if packet was queued for transmission.
     */
    bool send(const FullDataPacket& packet) {
        if (!_connected || !_udp) return false;

        // ---- Serialize packet ----
        // Option A: Nanopb (preferred, uncomment when .pb.h is generated)
        /*
        GloveData msg = GloveData_init_zero;
        msg.timestamp = packet.sensor.timestamp_us;

        // Hall features (15 floats)
        for (int i = 0; i < HALL_FEATURE_COUNT && i < 15; i++) {
            msg.hall_features[i] = packet.sensor.hall_xyz[i];
        }
        msg.hall_features_count = HALL_FEATURE_COUNT;

        // IMU features (6 floats)
        msg.imu_features[0] = packet.sensor.euler[0];
        msg.imu_features[1] = packet.sensor.euler[1];
        msg.imu_features[2] = packet.sensor.euler[2];
        msg.imu_features[3] = packet.sensor.gyro[0];
        msg.imu_features[4] = packet.sensor.gyro[1];
        msg.imu_features[5] = packet.sensor.gyro[2];
        msg.imu_features_count = IMU_FEATURE_COUNT;

        // L1 inference
        msg.l1_gesture_id = packet.inference.gesture_id;
        msg.l1_confidence = packet.inference.confidence;
        msg.l2_requested  = packet.inference.l2_requested;

        // Status
        const char* status_str = "STREAMING";
        switch (packet.status) {
            case FullDataPacket::STREAMING:       status_str = "STREAMING"; break;
            case FullDataPacket::MODEL_SWITCHING: status_str = "MODEL_SWITCHING"; break;
            case FullDataPacket::ERROR_STATE:     status_str = "ERROR"; break;
            case FullDataPacket::CALIBRATING:     status_str = "CALIBRATING"; break;
            case FullDataPacket::IDLE:            status_str = "IDLE"; break;
        }
        strncpy(msg.status, status_str, sizeof(msg.status) - 1);

        // Encode
        uint8_t buffer[256];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
        bool encoded = pb_encode(&stream, GloveData_fields, &msg);

        if (!encoded) {
            _packets_dropped++;
            return false;
        }

        return sendRaw(buffer, stream.bytes_written);
        */

        // ---- Option B: Compact binary format (fallback, no nanopb dependency) ----
        // Layout: [timestamp_u32][hall_15×f32][imu_6×f32][gesture_id_u32][conf_f32][flags_u8]
        // Total: 4 + 60 + 24 + 4 + 4 + 1 = 97 bytes
        uint8_t buffer[128];
        size_t offset = 0;

        // Timestamp (4 bytes)
        memcpy(buffer + offset, &packet.sensor.timestamp_us, 4); offset += 4;

        // Hall features (15 × 4 = 60 bytes)
        memcpy(buffer + offset, packet.sensor.hall_xyz, HALL_FEATURE_COUNT * sizeof(float));
        offset += HALL_FEATURE_COUNT * sizeof(float);

        // IMU features (6 × 4 = 24 bytes)
        float imu[IMU_FEATURE_COUNT];
        memcpy(imu, packet.sensor.euler, 3 * sizeof(float));
        memcpy(imu + 3, packet.sensor.gyro, 3 * sizeof(float));
        memcpy(buffer + offset, imu, IMU_FEATURE_COUNT * sizeof(float));
        offset += IMU_FEATURE_COUNT * sizeof(float);

        // Gesture ID (4 bytes)
        int32_t gid = packet.inference.gesture_id;
        memcpy(buffer + offset, &gid, 4); offset += 4;

        // Confidence (4 bytes)
        memcpy(buffer + offset, &packet.inference.confidence, 4); offset += 4;

        // Flags: bit0=l2_requested, bits7:1=status
        uint8_t flags = (packet.inference.l2_requested ? 0x01 : 0x00) |
                        (static_cast<uint8_t>(packet.status) << 1);
        buffer[offset++] = flags;

        _last_send_us = esp_timer_get_time();
        return sendRaw(buffer, offset);
    }

    // =========================================================================
    // Raw Send
    // =========================================================================

    /**
     * @brief Send raw bytes to the target.
     * @param data    Byte buffer.
     * @param length  Number of bytes to send.
     * @return true if packet was queued.
     */
    bool sendRaw(const uint8_t* data, size_t length) {
        if (!_connected || !_udp || !data || length == 0) return false;

        // Parse target IP string
        IPAddress targetAddr;
        if (!targetAddr.fromString(_target_ip)) {
            _packets_dropped++;
            return false;
        }

        size_t sent = _udp->writeTo(data, length, targetAddr, _target_port);
        if (sent == length) {
            _packets_sent++;
            return true;
        } else {
            _packets_dropped++;
            return false;
        }
    }

    // =========================================================================
    // Incoming Packet Handler
    // =========================================================================

    /**
     * @brief Handle incoming UDP packets (commands from edge server).
     *
     * Protocol:
     *   - 'S' → Start streaming
     *   - 'P' → Pause streaming
     *   - 'M<name>' → Switch model to <name>
     *   - 'R' → Reset Kalman filters
     */
    void handleIncomingPacket(AsyncUDPPacket& packet) {
        if (packet.length() < 1) return;

        char cmd = packet.data()[0];
        switch (cmd) {
            case 'S':
                Serial.println("[UDP] CMD: Start streaming");
                break;
            case 'P':
                Serial.println("[UDP] CMD: Pause streaming");
                break;
            case 'M':
                if (packet.length() > 1) {
                    char model_name[32] = {};
                    size_t len = std::min((size_t)(packet.length() - 1), sizeof(model_name) - 1);
                    memcpy(model_name, packet.data() + 1, len);
                    Serial.printf("[UDP] CMD: Switch model to '%s'\n", model_name);
                    // TODO: Forward to ModelRegistry::switchModel(model_name)
                }
                break;
            case 'R':
                Serial.println("[UDP] CMD: Reset filters");
                // TODO: Forward to SensorManager::resetFilters()
                break;
            default:
                Serial.printf("[UDP] Unknown CMD: 0x%02X\n", cmd);
                break;
        }
    }

    // =========================================================================
    // Status
    // =========================================================================

    bool isConnected() const { return _connected; }
    uint32_t packetsSent() const { return _packets_sent; }
    uint32_t packetsDropped() const { return _packets_dropped; }
    const char* targetIP() const { return _target_ip; }
    uint16_t targetPort() const { return _target_port; }

    /**
     * @brief Get WiFi signal strength (RSSI in dBm).
     * @return RSSI, or 0 if not connected.
     */
    int32_t getRSSI() const {
        return _connected ? WiFi.RSSI() : 0;
    }

private:
    AsyncUDP*  _udp;
    bool       _connected;
    char       _target_ip[16];   ///< Dotted-quad target IP
    uint16_t   _target_port;
    uint32_t   _packets_sent;
    uint32_t   _packets_dropped;
    uint32_t   _last_send_us;
};

#endif // UDP_TRANSMITTER_H
