#include <Arduino.h>
#include <motor.hpp>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <main_service_manager.hpp>
#include <arm_service_manager.hpp>
#include <linetracer_service_manager.hpp>

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);
Motor motorL, motorR;
//BLE reference
// https://lang-ship.com/reference/unofficial/M5StickC/Class/ESP32/BLEDevice/ 

//uuid generator
// https://www.uuidgenerator.net/ 

MainServiceManager* Main = nullptr;
ArmServiceManager* Arm = nullptr;
LineTracerServiceManager* LineTracer = nullptr;


BLEAdvertisedDevice* targetDevice;
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



void CommandFunc(const Command& command) {
    const char* TAG = "CommandCallback";
    ESP_LOGI(TAG, "");
}

void MotorFunc(const MotorData& motorData) {
    const char* TAG = "MotorCallback";
    ESP_LOGI(TAG, "motorL:%d, motorR:%d", motorData.power[0], motorData.power[1]);
    motorL.drive(motorData.power[0]);
    motorR.drive(motorData.power[1]);
}

void setup(void) {
    const char* TAG = "BLE Connection";
    Serial.begin(115200);
    motorL.attach(12, 14);
    motorR.attach(27, 26);
    Wire.begin();
    if (!bno.begin()) {
        ESP_LOGE(TAG, "Couldn't find bno device.");
        return;
    }

    BLEDevice::init("");
    BLEClient* pClient = BLEDevice::createClient();
    BLEScan* pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new advertisedDeviceCallbacks());
    pScan->setActiveScan(true);
    pScan->start(10);
    if (nullptr == targetDevice) {
        ESP_LOGI(TAG, "Couldn't find target device.");
        return;
    }
    pClient->connect(targetDevice);
    ESP_LOGI(TAG, "Device connected.");

    Main = MainServiceManager::getInstance();
    Main->init(pClient);
    Main->setName("LEGACY");
    Main->setMotorCallback(MotorFunc);

    Arm = ArmServiceManager::getInstance();
    Arm->init(pClient);
    Arm->setName("HUNTER");

    LineTracer = LineTracerServiceManager::getInstance();
    LineTracer->init(pClient);
    LineTracer->setName("THRESHOLD");
    
}



void loop(void) {
    BnoData bnoData;
    imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    bnoData.eular[0] = euler.x();
    bnoData.eular[1] = euler.y();
    bnoData.eular[2] = euler.z();
    bnoData.temp = bno.getTemp();  
    Main->setBnoData(bnoData);
    vTaskDelay(pdMS_TO_TICKS(100));
}
