#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <NimBLEDevice.h>
#include <WiFi.h>

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_PROVISION_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_DATA_UUID         "828917c1-ea55-4d4a-a66e-fd202cea0645"

class BLEManager {
public:
    String provisionedSSID = "";
    String provisionedPASS = "";
    bool isProvisioned = false;
    bool deviceConnected = false;

    class ServerCallbacks: public NimBLEServerCallbacks {
        BLEManager* manager;
    public:
        ServerCallbacks(BLEManager* mgr) : manager(mgr) {}
        void onConnect(NimBLEServer* pServer) {
            manager->deviceConnected = true;
            Serial.println("BLE Client Connected");
        };
        void onDisconnect(NimBLEServer* pServer) {
            manager->deviceConnected = false;
            Serial.println("BLE Client Disconnected");
            NimBLEDevice::startAdvertising();
        }
    };

    class ProvisionCallbacks: public NimBLECharacteristicCallbacks {
        BLEManager* manager;
    public:
        ProvisionCallbacks(BLEManager* mgr) : manager(mgr) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            if (value.length() > 0) {
                String payload = String(value.c_str());
                int splitIndex = payload.indexOf(':');
                if (splitIndex > 0) {
                    manager->provisionedSSID = payload.substring(0, splitIndex);
                    manager->provisionedPASS = payload.substring(splitIndex + 1);
                    manager->isProvisioned = true;
                    Serial.println("Received WiFi Credentials via BLE");
                }
            }
        }
    };

    void begin() {
        NimBLEDevice::init("Edge-AI-Glove");
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        NimBLEService *pService = pServer->createService(SERVICE_UUID);

        pProvisionChar = pService->createCharacteristic(
            CHAR_PROVISION_UUID,
            NIMBLE_PROPERTY::WRITE
        );
        pProvisionChar->setCallbacks(new ProvisionCallbacks(this));

        pDataChar = pService->createCharacteristic(
            CHAR_DATA_UUID,
            NIMBLE_PROPERTY::NOTIFY
        );

        pService->start();
        NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->start();
        Serial.println("BLE Advertising Started");
    }

    void notifyData(uint8_t* data, size_t length) {
        if (deviceConnected) {
            pDataChar->setValue(data, length);
            pDataChar->notify();
        }
    }

private:
    NimBLEServer* pServer;
    NimBLECharacteristic* pProvisionChar;
    NimBLECharacteristic* pDataChar;
};

#endif
