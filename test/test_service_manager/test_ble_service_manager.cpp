#include <Arduino.h>
#include <motor.hpp>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <main_service_manager.hpp>
#include <arm_service_manager.hpp>
#include <linetracer_service_manager.hpp>

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);
Motor motorL, motorR;

MainServiceManager* Main = nullptr;
ArmServiceManager* Arm = nullptr;
LineTracerServiceManager* LineTracer = nullptr;

const Command command_null = {.command = 0, .parameter = 0};
Command mainCommand, armCommand, lineTracerCommand;
bool bMainCommand, bArmCommand, bLineTracerCommand;
MotorData motorData;


imu::Vector<3> euler(0, 0, 0);
imu::Vector<3> eulerRef(0, 0, 0);

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



Command AnalyzeCommandData(uint8_t* data, size_t length) {
    const char* TAG = "CommandData";
    Command command;
    if (sizeof(Command) != length) {
        ESP_LOGE(TAG, "Eroor: characteristic data size doesn't match.");
        ESP_LOGE(TAG, "Expected data size:%d, Char data size:%d", sizeof(Command), length);
        return command;
    }
    memcpy(&command, data, sizeof(Command));
    return command;
} 

void MainCommandCallback(BLERemoteCharacteristic* pCommandChar, uint8_t* data, size_t length, bool isNotify) {
    const char* TAG = "Notify";
    ESP_LOGV(TAG, ">> notifyCallbackForCommandChar");
    mainCommand = AnalyzeCommandData(data, length);
    bMainCommand = true;
    ESP_LOGV(TAG, "notifyCallbackForCommandChar <<");
}



void MotorFunc(const MotorData _motorData) {
    const char* TAG = "MotorCallback";
    ESP_LOGI(TAG, "motorL:%d, motorR:%d", motorData.power[0], motorData.power[1]);
    motorData.
}


void bnoTask(void* pvParameters) {
    while (1) {
        euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER) - eulerRef;
        euler.x() = (euler.x() >= 0) ? euler.x() : euler.x() + 360;
        euler.y() = (euler.y() >= 0) ? euler.x() : euler.y() + 360;
        euler.z() = (euler.z() >= 0) ? euler.z() : euler.z() + 360;
        BnoData bnoData;
        bnoData.euler[0] = euler.x();
        bnoData.euler[1] = euler.y();
        bnoData.euler[2] = euler.z();
        bnoData.temp = bno.getTemp();
        Main->setBnoData(bnoData);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


void setup(void) {
    const char* TAG = "BLE Connection";
    Serial.begin(115200);
    motorL.attach(23, 25);
    motorR.attach(32, 33);
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
    Main->setNumOfMotor(2);
    Main->setMotorCallback(MotorFunc);
    Main->setCommandCallback(MainCommandCallback);

    Arm = ArmServiceManager::getInstance();
    Arm->init(pClient);
    Arm->setName("ANTITHESIS");

    LineTracer = LineTracerServiceManager::getInstance();
    LineTracer->init(pClient);
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);
    xTaskCreateUniversal(bnoTask, "bnoTask", 8192, NULL, 0, NULL, APP_CPU_NUM);
}


void loop(void) {
    if (bMainCommand) {
        digitalWrite(16, HIGH);
        bMainCommand = false;
        switch (mainCommand.command) {
            case 0:   //通常モード
                break;
            case 1: //ジャイロゼロ点調整コマンド
                eulerRef = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
                Main->setCommand(command_null);
                break;
        }
    }
    else {
        digitalWrite(16, LOW);
        digitalWrite(17, LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}
