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

const char arm_names[3][15] = {"ANTITHESIS", "MILKEYWAY"};

uint8_t armUnitNum_Old = 0;
uint8_t armUnitNum = 0;
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


void ArmCommandCallback(BLERemoteCharacteristic* pCommandChar, uint8_t *data, size_t length, bool isNotify) {
    const char* TAG = "Notify";
    ESP_LOGV(TAG, ">> notifyCallbackForCommandChar");
    armCommand = AnalyzeCommandData(data, length);
    bArmCommand = true;
    ESP_LOGV(TAG, "notifyCallbackForCommandChar <<");
}

void LineTracerCommandCallback(BLERemoteCharacteristic* pCommandChar, uint8_t *data, size_t length, bool isNotify) {
    const char* TAG = "Notify";
    ESP_LOGV(TAG, ">> notifyCallbackForCommandChar");
    lineTracerCommand = AnalyzeCommandData(data, length);
    bLineTracerCommand = true;
    ESP_LOGV(TAG, "notifyCallbackForCommandChar <<");
}

void MotorFunc(const MotorData _motorData) {
    const char* TAG = "MotorCallback";
    ESP_LOGI(TAG, "motorL:%d, motorR:%d", motorData.power[0], motorData.power[1]);
    motorData = _motorData;
    motorL.drive(motorData.power[0]);
    motorR.drive(motorData.power[1]);
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


#define END 0xC0
#define ESC 0xDB 
#define ESC_END 0xDC
#define ESC_ESC 0xDD

bool analyzeCommand(String packet, uint8_t& armUnitNum, ArmData& armData, uint8_t& ball_count) {
    const char* TAG = "analyze";
    for (int i = 0; i < packet.length(); i++) {
        if (packet.charAt(i) == ESC) {
            if (packet.charAt(i + 1) == ESC_END) {
                packet.setCharAt(i, END);
            }
            else if (packet.charAt(i + 1) == ESC_ESC) {
                packet.setCharAt(i, ESC);   
            }
            packet.remove(i + 1);
        }
    }
    armUnitNum = static_cast<uint8_t>(packet[0]);
    packet.remove(0, sizeof(uint8_t));
    memcpy(&armData, packet.substring(0, sizeof(ArmData) - 1).c_str(), sizeof(ArmData));
    packet.remove(0, sizeof(ArmData));
    if (packet.length() != sizeof(uint8_t)) {
        ESP_LOGW(TAG, "size of data is incorrect.:%d", packet.length());
        return false;
    }
    ball_count = (uint8_t)packet[0];
    ESP_LOGI(TAG, "ball_count: %d", ball_count);
    return true;
}


void armReceiveTask(void* pvParameters) { //arm->main->controller
    const char* TAG = "Recieve";
    Serial1.setTimeout(100);
    while (true) {
        String packet = Serial1.readStringUntil(END);
        if (packet != NULL) {
            ArmData armData;
            uint8_t ball_count;
            if (analyzeCommand(packet, armUnitNum, armData, ball_count)) {
                ESP_LOGI(TAG, "Successed to analyze command");
                Arm->setArmData(armData);
                Arm->setBallCount(ball_count);
            }
        }
        vTaskDelay(50);
    }
}



void sendToArmUnit(const Command command, const ArmData armData) {
    uint8_t packet[256];

    packet[0] = 0x01;

    uint8_t data1[sizeof(Command)];
    memcpy(data1, &command, sizeof(Command));
    for (int i = 0; i < sizeof(Command); i++) {
        packet[i + 1] = data1[i];
    }

    uint8_t data2[sizeof(ArmData)];
    memcpy(data2, &armData, sizeof(ArmData));
    for (int i = 0; i < sizeof(ArmData); i++) {
        packet[i + 1 + sizeof(Command)] = data2[i];
    }

    for (int i = 0; i < 1 + sizeof(Command) + sizeof(ArmData); i++) {
        if (packet[i] == END) {
            Serial1.write(ESC);
            Serial1.write(ESC_END);
        }
        else if (packet[i] == ESC) {
            Serial1.write(ESC);
            Serial1.write(ESC_ESC);
        }
        else {
            Serial1.write(packet[i]);
        }
    }
    Serial1.write(END);
}



void setup(void) {
    const char* TAG = "Legacy";
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial1.setPins(18, 19);
    motorL.attach(23, 25);
    motorR.attach(32, 33);
    Wire.begin();
    if (!bno.begin()) {
        ESP_LOGE(TAG, "Couldn't find bno device.");
        return;
    }

    xTaskCreateUniversal(armReceiveTask, "armReceive", 10000, NULL, 0, NULL, APP_CPU_NUM);

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
    Arm->setCommandCallback(ArmCommandCallback);

    LineTracer = LineTracerServiceManager::getInstance();
    LineTracer->init(pClient);
    LineTracer->setCommandCallback(LineTracerCommandCallback);
    xTaskCreateUniversal(bnoTask, "bno", 8192, NULL, 0, NULL, APP_CPU_NUM);
}



void loop(void) { //controller -> main (-> arm or linetracer)
    if (armUnitNum != armUnitNum_Old) {
        Arm->setName(arm_names[armUnitNum]);
        armUnitNum_Old = armUnitNum;
    }
    if (bMainCommand) {
        bMainCommand = false;
        switch (mainCommand.command) {
            case 0:   //通常
                break;
            case 1:   //ジャイロゼロ点調整
                eulerRef = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
                Main->setCommand(command_null);
                break;
        }
    }
    if (bArmCommand) {
        ESP_LOGI("test", "receieved arm command.");
        bArmCommand = false;
        ArmData armData;
        Arm->getCommand(armCommand);
        Arm->getArmData(armData);
        sendToArmUnit(armCommand, armData);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}
