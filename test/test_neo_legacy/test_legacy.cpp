#include <Arduino.h>
#include <motor.hpp>
#include <ESP32Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <main_service_manager.hpp>
#include <arm_service_manager.hpp>
#include <linetracer_service_manager.hpp>
#include "tb6612fng.hpp"
#include "new_elevator.hpp"

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);
Motor motorL, motorR;
tb6612fng tb6612;
Elevator *elevator;

MainServiceManager* Main = nullptr;
ArmServiceManager* Arm = nullptr;
LineTracerServiceManager* LineTracer = nullptr;

const Command command_null = {.command = 0, .parameter = 0};
Command mainCommand, armCommand, lineTracerCommand;
bool bMainCommand, bArmCommand, bLineTracerCommand;

MotorData motorData;

imu::Vector<3> euler(0, 0, 0);
imu::Vector<3> eulerRef(0, 0, 0);

constexpr uint8_t SERVO_ARM_PIN = 26;
constexpr uint8_t SERVO_GATE_PIN = 27;

Servo servo_arm, servo_gate;
constexpr uint8_t arm_laser_pin = 16;
constexpr uint8_t elevartor_laser_pin = 17;
constexpr uint8_t arm_ball_detection_pin = 18;
constexpr uint8_t elevetor_ball_detection_pin = 19;

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


uint8_t ball_count = 0;
void elevator_task(void* pvParameters) {
    const char* TAG = "Elevator";
    while (true) {
        if (0 == elevator->getState()) {
            if (digitalRead(elevetor_ball_detection_pin) == HIGH) {
                ESP_LOGI(TAG, "Detected ball.");
                if (ball_count == 5) {
                    ESP_LOGW(TAG, "Ball tank is already filled.");
                }
                else {
                    elevator->rise();
                }
            }
        }
        else if (1 == elevator->getState()) {
            // servo_elevetor.write(90 - 38);
            ball_count++;
            Arm->setBallCount(ball_count);
            ESP_LOGI(TAG, "Ball Tank Usage:%d", ball_count);
            vTaskDelay(pdMS_TO_TICKS(300));
            elevator->fall();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


inline void ball_catch(const int begin_angle) {
    servo_arm.write(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    servo_arm.write(begin_angle);
}

inline void ball_release(void) {
    servo_gate.write(130);
    vTaskDelay(600);
    servo_gate.write(30);
    vTaskDelay(600);
    ball_count--;
    Arm->setBallCount(ball_count);
    ESP_LOGI("ball_release", "%d", ball_count);
}


void ball_task(void* pvParameters) {
    const char* TAG = "ball";
    enum Mode_t {CATCH, RELEASE};
    Mode_t mode = CATCH;
    int begin_angle = 180;
    while (1) {
        switch (mode) {
            case CATCH:
                if (digitalRead(arm_ball_detection_pin) == HIGH) {
                    ESP_LOGI(TAG, "arm detected ball.");
                    ball_catch(begin_angle);
                }
                if (bArmCommand) {
                    bArmCommand = false;
                    if (armCommand.command == 1) {
                        if (armCommand.parameter == 0) {
                            if (begin_angle <= 170) {
                                begin_angle += 10;
                            }
                            servo_arm.write(begin_angle);
                        }
                        else if (armCommand.parameter == 2) {
                            if (begin_angle >= 10) {
                                begin_angle -= 10;
                            }
                            servo_arm.write(begin_angle);
                        }
                    }
                    if (armCommand.command == 2) {
                        ball_catch(begin_angle);
                    }
                    if (armCommand.command == 3) {
                        mode = RELEASE;
                        servo_arm.write(0);
                        digitalWrite(arm_laser_pin, LOW);
                    }
                }
                break;

            case RELEASE:
                if (bArmCommand) {
                    bArmCommand = false;
                    if (armCommand.command == 1) {
                        ball_release();
                    }
                    if (armCommand.command == 3) {
                        mode = CATCH;
                        servo_arm.write(180);
                        digitalWrite(arm_laser_pin, HIGH);
                    }
                }
                break;
        }
        armCommand = command_null;
        Arm->setCommand(command_null);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void setup(void) {
    const char* TAG = "Legacy";
    Serial.begin(115200);
    pinMode(arm_laser_pin, OUTPUT);
    pinMode(elevartor_laser_pin, OUTPUT);
    pinMode(arm_ball_detection_pin, INPUT_PULLUP);
    pinMode(elevetor_ball_detection_pin, INPUT_PULLUP);
    motorL.attach(23, 25);
    motorR.attach(32, 33);
    servo_arm.attach(SERVO_ARM_PIN);
    servo_gate.attach(SERVO_GATE_PIN);
    elevator = new Elevator(12, 13, 14, 15, 1000);
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
    pScan->start(20);
    if (nullptr == targetDevice) {
        ESP_LOGI(TAG, "Couldn't find target device.");
        return;
    }
    pClient->connect(targetDevice);
    ESP_LOGI(TAG, "Device connected.");

    Main = MainServiceManager::getInstance();
    Main->init(pClient);
    Main->setName("LEGACY Rev 1");
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
    xTaskCreateUniversal(elevator_task, "elevator", 8192, NULL, 0, NULL, APP_CPU_NUM);
    xTaskCreateUniversal(ball_task, "release", 8192, NULL, 0, NULL, APP_CPU_NUM);
}



void loop(void) {
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
    vTaskDelay(pdMS_TO_TICKS(10));
}
