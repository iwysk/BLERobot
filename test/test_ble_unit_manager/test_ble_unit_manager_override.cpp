#include <Arduino.h>
#include "main_unit_manager.hpp"
#include "arm_unit_manager.hpp"
#include "linetracer_unit_manager.hpp"

bool deviceConnected = false;
bool bMainUnit = false;
bool bLineTracerUnit = false;
bool bArmUnit = false;

const BLEUUID main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
BLEClient* pClient;
BLEAdvertisedDevice *targetDevice;

class advertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    const char* TAG = "BLE Callbacks";
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        ESP_LOGI(TAG, "Found new device:, %s", advertisedDevice.toString().c_str());
        if (advertisedDevice.isAdvertisingService(main_service_uuid)) {
            ESP_LOGI(TAG, "found target device.");
            targetDevice = new BLEAdvertisedDevice(advertisedDevice);
            BLEDevice::getScan()->stop();
        }
    }
};

MainUnitManager* mainUnit = nullptr;
ArmUnitManager* armUnit = nullptr;
LineTracerUnitManager* lineTracerUnit = nullptr;

BnoData bnoData;
MotorData motorData;
uint8_t main_battery_level = 0;

ArmData armData;
uint8_t ball_count = 0;
uint8_t arm_battery_level = 0;

LineData lineData;
uint8_t linetracer_battery_level = 0;


void setup(void) {
    const char* TAG = "setup";
    Serial.begin(115200);
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    BLEScan *pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new advertisedDeviceCallbacks());
    pScan->setActiveScan(true);
    pScan->start(5, false);
    if (targetDevice == nullptr) {
        ESP_LOGE(TAG, "couldn't find target device.");
    }
    else {
        if (pClient->connect(targetDevice)) {
            ESP_LOGI(TAG, "Client connected to target device.");
            deviceConnected = true;
        }
        else {
            ESP_LOGW(TAG, "Failed to connect to target device.");
        }
    }

    if (deviceConnected) { //サービスおよび特性のチェック
        mainUnit = MainUnitManager::getInstance(pClient, true, bMainUnit);
        armUnit = ArmUnitManager::getInstance(pClient, true, bArmUnit);
        lineTracerUnit = LineTracerUnitManager::getInstance(pClient, true, bLineTracerUnit);

        mainUnit->getMotorData(motorData);
        motorData.power[0] = 255; motorData.power[1] = -255;
        mainUnit->setMotorData(motorData);

        mainUnit->getEular(bnoData.Eular);
        mainUnit->getTemp(bnoData.temp);
        mainUnit->getBatteryLevel(main_battery_level);

        armUnit->getArmData(armData);
        armUnit->getBallCount(ball_count);
        armUnit->getBatteryLevel(arm_battery_level);

        lineTracerUnit->getLineData(lineData);
        lineTracerUnit->getBatteryLevel(linetracer_battery_level);
    }
}


void loop(void) {
    if (deviceConnected && mainUnit->getInitialized()) {
        //mainUnit->getBnoData(bnoData);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}