#ifndef WS_MANAGER_H
#define WS_MANAGER_H

#include <WebSocketsServer.h>
#include <WiFi.h>

class WSManager {
public:
    WSManager(uint16_t port = 81) : webSocket(port) {}

    void begin() {
        webSocket.begin();
        webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
            this->webSocketEvent(num, type, payload, length);
        });
        Serial.println("WebSocket Server Started");
    }

    void loop() {
        webSocket.loop();
    }

    void broadcastMessage(uint8_t* payload, size_t length) {
        webSocket.broadcastBIN(payload, length);
    }

private:
    WebSocketsServer webSocket;

    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
        switch(type) {
            case WStype_DISCONNECTED:
                Serial.printf("[%u] Disconnected!\n", num);
                break;
            case WStype_CONNECTED:
                {
                    IPAddress ip = webSocket.remoteIP(num);
                    Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
                }
                break;
            case WStype_TEXT:
            case WStype_BIN:
                break;
        }
    }
};

#endif
