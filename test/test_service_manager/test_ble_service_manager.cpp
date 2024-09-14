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


imu::Vector<3> euler(0, 0, 0);
imu::Vector<3> eulerRef(0, 0, 0);

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


void bnoTask(void* pvParameters) {
    while (1) {
        BnoData bnoData;
        euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER) - eulerRef;
        euler.x() = (euler.x() >= 0) ? euler.x() : euler.x() + 360;
        euler.y() = (euler.y() >= 0) ? euler.x() : euler.y() + 360;
        euler.z() = (euler.z() >= 0) ? euler.z() : euler.z() + 360;
        bnoData.euler[0] = euler.x();
        bnoData.euler[1] = euler.y();
        bnoData.euler[2] = euler.z();
        bnoData.temp = bno.getTemp();  
        Main->setBnoData(bnoData);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


void GyroForward(const float targetAngle) {

}


void forward(const int power) {
    motorL.drive(power);
    motorR.drive(power);
}

void accelarate(const int &power, const int &timeForAcceralate, const int &timeForRun) {
    int period = timeForAcceralate * 1000 / 50;
    for (int i = 0; i < 50; i++) {
        forward((float)power / 50.0f * (float)i);
        delayMicroseconds(period);
    }
    forward(power);
    vTaskDelay(pdMS_TO_TICKS(timeForRun));
    for (int i = 50; i > 0; i--) {
        forward((float)power / 50.0f * (float)i);
        delayMicroseconds(period);
    }
}

void GyroPIDTurn(void* pvParameters) {
    const char* TAG = "PIDTurn";
    ESP_LOGI(TAG, ">> GyroPIDTurn");
    PIDParameters *pParameters = static_cast<PIDParameters*>(pvParameters);
    PIDParameters parameters = *pParameters;
    TickType_t start = xTaskGetTickCount();
    unsigned int count = 0;
    float timegain = 0;
    ESP_LOGI(TAG, "target: %f", parameters.target);
    while (pdMS_TO_TICKS(xTaskGetTickCount() - start) <= 3000) {
        float diff = euler.x() - parameters.target;
        if (timegain <= 1) {
            timegain += 0.02;
        }
        if (diff <= -180) {
            diff = 360 + diff;
        } else if (diff >= 180) {
            diff = diff - 360;
        }
        ESP_LOGI(TAG, "diff: %f", diff);
        int throttle = diff * parameters.pGain * timegain;
        motorL.drive(-throttle);
        motorR.drive(throttle);
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


void gyroPIDTurn_Block(const PIDParameters parameters) {
    const char* TAG = "PIDTurn_Block";
    ESP_LOGI(TAG, ">> GyroPIDTurn_Block");
    TickType_t start = xTaskGetTickCount();
    unsigned int count = 0;
    float timegain = 0;
    ESP_LOGI(TAG, "target: %f", parameters.target);
    while (pdMS_TO_TICKS(xTaskGetTickCount() - start) <= 3000) {
        float diff = euler.x() - parameters.target;
        if (timegain <= 1) {
            timegain += 0.02;
        }
        if (diff <= -180) {
            diff = 360 + diff;
        } else if (diff >= 180) {
            diff = diff - 360;
        }
        ESP_LOGI(TAG, "diff: %f", diff);
        int throttle = diff * parameters.pGain * timegain; 
        motorL.drive(-throttle);
        motorR.drive(throttle);
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
    ESP_LOGI(TAG, "GyroPIDTurn_Block <<"); //終了 
}



void AutomaticMovement(void *pvPamaremeters) {
    if (direction == 0) { //往路
        accelarate(100, 200, 800);
        PIDParameters parameters = {.target = 90.0,
                                            .pGain = 1.5,
                                            .iGain = 0.001,
                                            .dGain = 0.02};
        gyroPIDTurn_Block(parameters);
        accelarate(130, 400, 300);
        parameters.target = 180;
        gyroPIDTurn_Block(parameters);
        accelarate(130, 500, 4000);
    }
    if (direction == 1) { //復路
        accelarate(130, 500, 4000);
        PIDParameters parameters = {.target = 270.0,
                                            .pGain = 1.5,
                                            .iGain = 0.001,
                                            .dGain = 0.02};
        gyroPIDTurn_Block(parameters);
        accelarate(130, 400, 300);
        parameters.target = 180;
        gyroPIDTurn_Block(parameters);
        accelarate(130, 500, 1000);
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
            case 0:
                
                break;
            case 1: {
                digitalWrite(17, HIGH);
                PIDParameters parameters = {.target = ((float)mainCommand.parameter) * 90.0f,
                                            .pGain = 1.5,
                                            .iGain = 0.001,
                                            .dGain = 0.02};
                xTaskCreateUniversal(GyroPIDTurn, "GyroPIDTurn", 4096, &parameters, 1, nullptr, APP_CPU_NUM);
                break;
            }
            case 2:
                eulerRef = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
                Main->setCommand(command_null);
                break;
            case 3:
                AutomaticMovement(mainCommand.parameter);
                Main->setCommand(command_null);

        }
    }
    else {
        digitalWrite(16, LOW);
        digitalWrite(17, LOW);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}
