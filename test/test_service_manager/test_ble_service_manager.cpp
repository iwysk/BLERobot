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

const Command command_null = {.command = 0, .parameter = 0};
Command mainCommand, armCommand, lineTracerCommand;
bool bMainCommand, bArmCommand, bLineTracerCommand;

struct PIDParameters {
    float target;
    float pGain, iGain, dGain;
};

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


void MotorFunc(const MotorData motorData) {
    const char* TAG = "MotorCallback";
    ESP_LOGI(TAG, "motorL:%d, motorR:%d", motorData.power[0], motorData.power[1]);
    motorL.drive(motorData.power[0]);
    motorR.drive(motorData.power[1]);
}

void GyroPIDTurn(void* pvParameters);

void setup(void) {
    const char* TAG = "BLE Connection";
    Serial.begin(115200);
    motorL.attach(23, 25);
    motorR.attach(26, 27);
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
    Arm->setName("HUNTER");

    LineTracer = LineTracerServiceManager::getInstance();
    LineTracer->init(pClient);
    LineTracer->setName("THRESHOLD");
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);

}


imu::Vector<3> euler(0, 0, 0);
imu::Vector<3> eularRef(0, 0, 0);

void loop(void) {
    BnoData bnoData;
    euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER) - eularRef;
    bnoData.eular[0] = euler.x();
    bnoData.eular[1] = euler.y();
    bnoData.eular[2] = euler.z();
    bnoData.temp = bno.getTemp();  
    Main->setBnoData(bnoData);
    if (bMainCommand) {
        digitalWrite(16, HIGH);
        bMainCommand = false;
        switch (mainCommand.command) {
            case 0:
                break;
            case 1: {
                digitalWrite(17, HIGH);
                PIDParameters parameters = {.target = (float)mainCommand.parameter * 90,
                                            .pGain = 0.6,
                                            .iGain = 0.001,
                                            .dGain = 0.02};
                xTaskCreateUniversal(GyroPIDTurn, "GyroPIDTurn", 4096, &parameters, 1, nullptr, APP_CPU_NUM);
                break;
            }
            case 2:
                eularRef = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
                Main->setCommand(command_null);
                break;
            
        }
    }
    else {
        digitalWrite(16, LOW);
        digitalWrite(17, LOW);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
}


void GyroPIDTurn(void* pvParameters) {
    const char* TAG = "PIDTurn";
    ESP_LOGI(TAG, ">> GyroPIDTurn");
    PIDParameters *parameters = static_cast<PIDParameters*>(pvParameters);
    TickType_t start = xTaskGetTickCount();
    unsigned int count = 0;
    while (pdMS_TO_TICKS(xTaskGetTickCount() - start) <= 3000) {
        float diff = euler.x() - parameters->target;
        if (diff <= -180) {
            diff = 360 + diff;
        } else if (diff >= 180) {
            diff = diff - 360;
        }
        ESP_LOGI(TAG, "diff: %d", diff);
        int throttle = diff * parameters->iGain; 
        motorL.drive(throttle);
        motorR.drive(-throttle);
        if (abs(diff) <= 2) {
            count++;
        } else {
            count = 0;
        }
        if (count == 20) {
            ESP_LOGI(TAG, "Successed to lock on target angle.");
            break;
        } 
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "GyroPIDTurn <<"); //終了 
    Main->setCommand(command_null); 
    vTaskDelete(NULL);
}
