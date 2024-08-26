#include <Arduino.h>
#include <seven_segment.hpp>
#include <main_service.hpp>
#include <arm_service.hpp>
#include <linetracer_service.hpp>
#include "tftfunc.hpp"


constexpr uint8_t THROTTLE_PIN = 36;
constexpr uint8_t STEERING_PIN = 39;
constexpr uint8_t MODE_SELECT_PIN = 25;
constexpr uint8_t UP_PIN = 22;
constexpr uint8_t DOWN_PIN = 26;
constexpr uint8_t LEFT_PIN = 33;
constexpr uint8_t RIGHT_PIN = 32;

SevenSegment *seg;
MainService *Main;
ArmService *Arm;
LineTracerService *LineTracer;

volatile bool isConnected = true;
volatile bool nowConnected = false;
BnoData bnoData;

QueueHandle_t buttonQueue;
struct ButtonData {
    bool mode_select_pin;
    bool up_pin;
    bool down_pin;
    bool left_pin;
    bool right_pin;
};


GyroCompass compass;


class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        ESP_LOGI("BLE", "Device connected.");
        isConnected = true;
        nowConnected = true;
    }
    void onDisconnect(BLEServer *pServer) override {
        BLEDevice::startAdvertising();
        ESP_LOGI("BLE", "Device disconnected.");
        isConnected = false;
    }
};


void batteryLevelFunc(const char* type, const uint8_t& battery_level) {
    ESP_LOGI("battery_level", "%s: %d", type, battery_level);
}

void bnoFunc(const BnoData& _bnoData) {
    bnoData = _bnoData;
    ESP_LOGI("bnoFunc", "x:%f y:%f z:%f temp:%d", bnoData.eular[0], bnoData.eular[1], bnoData.eular[2], bnoData.temp);
}

void ballCountFunc(const uint8_t& ball_count) {
    ESP_LOGI("ballcount", "%d", ball_count);
    seg->Number(ball_count);
}

void readModeSelectPin(void* pvParameters);

void setup(void) {
    pinMode(THROTTLE_PIN, ANALOG);
    pinMode(STEERING_PIN, ANALOG);
    pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
    pinMode(UP_PIN, INPUT_PULLUP);
    pinMode(DOWN_PIN, INPUT_PULLUP);
    pinMode(LEFT_PIN, INPUT_PULLUP);
    pinMode(RIGHT_PIN, INPUT_PULLUP);
    seg = new SevenSegment(5, 12, 13, 14, 16, 17, 21, 21); //なんか19番ピンだと動かない←TFTのMISOだった
    seg->Number(8, 1);

    BLEDevice::init("GO108");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    Main = MainService::getInstance();
    Main->init(pServer);
    Main->setBnoCharCallback(bnoFunc);
    Main->setBatteryLevelCharCallback(batteryLevelFunc);
    Main->startService();

    Arm = ArmService::getInstance();
    Arm->init(pServer);
    Arm->setBallCountCharCallback(ballCountFunc);
    Arm->setBatteryLevelCharCallback(batteryLevelFunc);
    Arm->startService();

    LineTracer = LineTracerService::getInstance();
    LineTracer->init(pServer);
    LineTracer->setBatteryLevelCharCallback(batteryLevelFunc);
    LineTracer->startService();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(main_service_uuid);
    pAdvertising->start();

    buttonQueue = xQueueCreate(1, sizeof(ButtonData));
    xTaskCreateUniversal(readModeSelectPin, "readModeSelectPin", 1024, nullptr, 0, nullptr, APP_CPU_NUM);
    initTFT();
    if (isConnected) { //ブートログ中に接続した場合
        nowConnected = false;
        showConnection(isConnected, 2000);
        showServiceName(Main, Arm, LineTracer, 3000);
    }
}



void loop(void) {
    if (isConnected) {
        enum ControlMode{Normal, GyroAssist, LineTrace, Auto};
        static ControlMode mode = Normal;
        static bool isSwitched = true;  //モード切替の時のみ
        static MotorData motorData, motorData_old;
        
        if (nowConnected) {
            nowConnected = false;
            isSwitched = true; //描画のため
            showConnection(isConnected, 1000);
        }

        uint32_t Volt_Throttle = analogReadMilliVolts(THROTTLE_PIN);
        uint32_t Volt_Steering = analogReadMilliVolts(STEERING_PIN);

        int throttle_power, steering_power;
        ButtonData buttonData = {.mode_select_pin = 0, 
                                 .up_pin = 0,
                                 .down_pin = 0,
                                 .left_pin = 0,
                                 .right_pin = 0};
        if (pdTRUE == xQueueReceive(buttonQueue, &buttonData, 0)) {
            ESP_LOGI("loop", "received a queue. button_state: %d, %d, %d, %d, %d",
                buttonData.mode_select_pin, buttonData.up_pin, buttonData.down_pin, buttonData.left_pin, buttonData.right_pin);
        }

        switch (mode) {
            case Normal:
                if (isSwitched) {
                    tft.fillScreen(TFT_BLACK);
                    tft.setTextSize(4);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_WHITE);
                    tft.println("Normal Mode");
                    compass.init(tft.width() / 2, tft.height() / 2, 90, TFT_GREEN);
                    compass.draw(tft, bnoData.eular, true);
                    isSwitched = false;
                }
                if (buttonData.mode_select_pin) {
                    mode = GyroAssist;
                    isSwitched = true;
                    break;
                }
                Command command;
                Main->getCommand(command);
                if (command.command == 0) { //通常状態
                    throttle_power = map(Volt_Throttle, 1650, 2200, -255, 255);
                    throttle_power = constrain(throttle_power, -200, 200);
                    if (abs(throttle_power) <= 30) {
                        throttle_power = 0;
                    }
                    steering_power = map(Volt_Steering, 1100, 2200, -50, 50);
                    steering_power = constrain(steering_power, -50, 50);
                    if (abs(steering_power) <= 10) {
                        steering_power = 0;
                    }
                    motorData.power[0] = throttle_power  + steering_power;
                    motorData.power[1] = throttle_power - steering_power;
                    if (motorData != motorData_old) {
                        Main->setMotorData(motorData);
                        tft.setTextSize(2);
                        tft.fillRect(tft.width() - tft.textWidth("L:     R:    "), tft.height() - tft.fontHeight(), tft.textWidth("L:    R:    "), tft.fontHeight(), TFT_BLACK);
                        tft.setCursor(tft.width() - tft.textWidth("L:     R:    "), tft.height() - tft.fontHeight());
                        tft.print("L:");
                        tft.print(motorData.power[0]);
                        tft.setCursor(tft.width() - tft.textWidth("R:    "), tft.height() - tft.fontHeight());
                        tft.print("R:");
                        tft.print(motorData.power[1]);
                    }
                }

                compass.draw(tft, bnoData.eular);

                if (buttonData.up_pin == 1) { //４方向の旋回
                    command.command = 1;
                    command.parameter = 0;
                    Main->setCommand(command);
                }
                if (buttonData.down_pin == 1) { 
                    command.command = 1;
                    command.parameter = 1;
                    Main->setCommand(command);
                }
                if (buttonData.left_pin == 1) { 
                    command.command = 1;
                    command.parameter = 2;
                    Main->setCommand(command);
                }
                if (buttonData.right_pin == 1) { 
                    command.command = 1;
                    command.parameter = 3;
                    Main->setCommand(command);
                }
                break;

            case GyroAssist:
                if (isSwitched) {
                    tft.fillScreen(TFT_BLACK);
                    tft.setTextSize(3);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_YELLOW);
                    tft.println("GyroAssist Mode");
                    isSwitched = false;
                }
                if (buttonData.mode_select_pin) {
                    mode = Auto;
                    isSwitched = true;
                    break;
                }
                break;

            case LineTrace:
                if (isSwitched) {
                    tft.fillScreen(TFT_GREEN);
                    tft.setTextSize(3);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_WHITE);
                    tft.println("LineTrace Mode");
                    isSwitched = false;
                }
                if (buttonData.mode_select_pin) {
                    mode = Auto;
                    isSwitched = true;
                    break;
                }
                break;

            case Auto:
                if (isSwitched) {
                    tft.fillScreen(TFT_DARKCYAN);
                    tft.setTextSize(4);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_WHITE);
                    tft.println("Auto Mode");
                    isSwitched = false;
                }
                if (buttonData.mode_select_pin) {
                    mode = Normal;
                    isSwitched = true;
                    break;
                }
                break;

        }
        motorData_old = motorData;
    }
    else {
        showConnection(isConnected, 1000);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}



void readModeSelectPin(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t state_old[5];
    while (true) {
        bool send_flag = false;
        ButtonData buttonData = {.mode_select_pin = 0, 
                                 .up_pin = 0,
                                 .down_pin = 0,
                                 .left_pin = 0,
                                 .right_pin = 0};
        uint8_t state[5];
        state[0] = digitalRead(MODE_SELECT_PIN);
        state[1] = digitalRead(UP_PIN);
        state[2] = digitalRead(DOWN_PIN);
        state[3] = digitalRead(RIGHT_PIN);
        state[4] = digitalRead(LEFT_PIN);
        for (byte i = 0; i < 5; i++) {
            if (state[i] == LOW && state_old[i] == HIGH) {
                send_flag = true; //1つでもボタンが押されればキューを送信する
                switch(i) {
                    case 0:
                        buttonData.mode_select_pin = 1;
                        break;
                    case 1:
                        buttonData.up_pin = 1;
                        break;
                    case 2:
                        buttonData.down_pin = 1;
                        break;
                    case 3:
                        buttonData.left_pin = 1;
                        break;
                    case 4:
                        buttonData.right_pin = 1;
                }
            }
            state_old[i] = state[i];
        }
        if (send_flag) {
            xQueueOverwrite(buttonQueue, &buttonData); //常にpdPASSを返すらしい
        }
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}