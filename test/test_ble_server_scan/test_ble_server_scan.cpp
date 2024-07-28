#include <Arduino.h>
#include <esp_log.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>


BLEUUID base_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
BLEAdvertisedDevice *targetDevice;

typedef enum {scanning, doConnect, connected} state_t; 
volatile state_t mode = scanning;
BLEClient *pTargetClient;


// class ClientCallbacks : public BLEClientCallbacks {
//     void onConnect(BLEClient *pClient) {
//         log_i("device connected");
//     }
//     void onDisonnect(BLEClient *pClient) {
//         log_i("device disconnected");
//     }
// };


class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        std::string dev_info = advertisedDevice.toString();
        log_i("New device found: %s", dev_info.c_str());
        if (advertisedDevice.haveServiceUUID()) {
            if (advertisedDevice.isAdvertisingService(base_service_uuid)) {
                log_i("found target device.");
                mode = doConnect;
                targetDevice = new BLEAdvertisedDevice(advertisedDevice);
            }
        }
    }
};


void ServerStructScan(BLEClient* pClient) {
    uint16_t handle;
    BLEUUID uuid;
    auto services = pClient->getServices();
    for (auto servicePair = services->begin(); servicePair != services->end(); servicePair = std::next(servicePair, 1)) {
        //printf("service: %s\r\n", servicePair->first.c_str());
        BLERemoteService *pRemoteService = servicePair->second;
        handle = pRemoteService->getHandle();
        uuid = pRemoteService->getUUID();
        printf("service: handle: %04x, uuid: %s\n", handle, uuid.toString().c_str());
        auto chars = pRemoteService->getCharacteristics();
        for (auto charPair = chars->begin(); charPair != chars->end(); charPair = std::next(charPair, 1)) {
            //printf("    characteristic: %s\r\n", charPair->first.c_str());
            BLERemoteCharacteristic *pRemoteCharacteristic = charPair->second;
            handle = pRemoteCharacteristic->getHandle();
            uuid = pRemoteCharacteristic->getUUID();
            uint8_t *data = pRemoteCharacteristic->readRawData();
            printf("     characteristic: handle: %04x, uuid: %s\n", handle, uuid.toString().c_str());
        }
    }
}

// void ServerStructScanVervose(BLEClient* pClient) {
//     std::map<std::string, BLERemoteService*> *services = pClient->getServices();
//     for (auto servicePair : *services) {
//         printf("service: %s\r\n", servicePair.first.c_str());
//         BLERemoteService service = servicePair    }
// }


void setup(void) {
    Serial.begin(115200);
    BLEDevice::init("");
    pTargetClient = BLEDevice::createClient();
    BLEScan *pScan = BLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
    pScan->start(5);
}

void loop(void) {
    switch(mode) {
        case scanning:
            break;

        case doConnect:
            if (pTargetClient->connect(targetDevice)) {
                log_i("Successed to connect target device.");
                mode = connected;
            }
            else {
                log_i("Failed to connect target device.");
                mode = scanning;
            }
            break;

        case connected:
            std::string gatt_server_info = pTargetClient->toString();
            log_i("server info: %s", gatt_server_info.c_str());
            ServerStructScan(pTargetClient);
            while (1) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            //pTargetClient->disconnect();
            break;
    }
}
