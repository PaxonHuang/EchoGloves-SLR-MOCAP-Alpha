#ifndef UDP_TRANSMITTER_H
#define UDP_TRANSMITTER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <pb_encode.h>
#include "glove_data.pb.h"

class UDPTransmitter {
public:
    UDPTransmitter(const char* ssid, const char* password, const char* host, uint16_t port)
        : ssid(ssid), password(password), host(host), port(port) {}

    void begin() {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi connected");
        udp.begin(WiFi.localIP(), port);
    }

    bool send(const GloveData& data) {
        uint8_t buffer[256];
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        if (!pb_encode(&stream, GloveData_fields, &data)) {
            Serial.println("Encoding failed");
            return false;
        }

        udp.beginPacket(host, port);
        udp.write(buffer, stream.bytes_written);
        return udp.endPacket();
    }

private:
    const char* ssid;
    const char* password;
    const char* host;
    uint16_t port;
    WiFiUDP udp;
};

#endif
