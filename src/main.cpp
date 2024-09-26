#include <Arduino.h>
#include <seven_segment.hpp>
#include <main_service.hpp>
#include <arm_service.hpp>
#include <linetracer_service.hpp>
#include <pidcontrol.hpp>
#include "tftfunc.hpp"

const Command command_null = {.command = 0, .parameter = 0};

constexpr uint8_t THROTTLE_PIN = 36;
constexpr uint8_t STEERING_PIN = 39;
constexpr uint8_t MODE_SELECT_PIN = 25;
constexpr uint8_t UP_PIN = 22;
constexpr uint8_t LEFT_PIN = 33;
constexpr uint8_t DOWN_PIN = 26;
constexpr uint8_t RIGHT_PIN = 32;
constexpr uint8_t JOYSTICK_BUTTON_PIN = 27;
constexpr uint8_t JOYSTICK_X_PIN = 35;
constexpr uint8_t JOYSTICK_Y_PIN = 34;

SevenSegment *seg;
MainService *Main;
ArmService *Arm;
LineTracerService *LineTracer;

volatile bool isConnected = false;     //////////////////////////////////////////////////
volatile bool nowConnected = false;

MotorData motorData = {.power = {0, 0, 0, 0}};
BnoData bnoData = {.euler = {0.0, 0.0, 0.0}, .temp = 0};

struct ButtonData {
    uint8_t mode_select_pin;
    uint8_t up_pin;
    uint8_t left_pin;
    uint8_t down_pin;
    uint8_t right_pin;
    uint8_t joystick_pin;
};


QueueHandle_t joyStickQueue;
enum JoystickState{ELSE, UP, DOWN, LEFT, RIGHT};
JoystickState jState;

GyroCompass compass;
PIDParameters parameters = {.pGain = 1,
                            .iGain = 0.001,
                            .dGain = 0.02};

struct PIDTurnParameters {
    PIDParameters parameters;
    double target;
    unsigned long timeout;
};


PIDTurnParameters turnParameters = {.parameters = parameters,
                                    .target = 0,
                                    .timeout = 3000};

PIDControl PID = PIDParameters(parameters);

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
}

void ballCountFunc(const uint8_t& ball_count) {
    ESP_LOGI("ballcount", "%d", ball_count);
    seg->Number(ball_count);
}

void readButton(void* pvParameters);

void setup(void) {
    pinMode(THROTTLE_PIN, ANALOG);
    pinMode(STEERING_PIN, ANALOG);
    pinMode(MODE_SELECT_PIN, INPUT_PULLUP);
    pinMode(UP_PIN, INPUT_PULLUP);
    pinMode(DOWN_PIN, INPUT_PULLUP);
    pinMode(LEFT_PIN, INPUT_PULLUP);
    pinMode(RIGHT_PIN, INPUT_PULLUP);
    pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    seg = new SevenSegment(17, 21, 13, 14, 16, 5, 12); //なんか19番ピンだと動かない←TFTのMISOだった
    seg->Number(0);

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

    initTFT();
}

void Steering(const int throttle, const int steering) {
    motorData.power[0] = throttle - steering;
    motorData.power[1] = throttle + steering;
    constrain(motorData.power[0], -200, 200);
    constrain(motorData.power[1], -200, 200);
}

double getGyroError(const double target) {
    double error = bnoData.euler[0] - target;
    if (error <= -180) {
        error = error + 360;
    } else if (error >= 180) {
        error = error - 360;
    }
    return error;
}

void gyroPIDTurn(const double target, const unsigned long timeout) {
    const char* TAG = "PID";
    unsigned long start = millis();
    unsigned int count;
    double timegain = 0;
    PIDControl PID = PIDControl(parameters);
    while (millis() - start <= timeout) {
        if (timegain <= 1) {
            timegain += 0.02;
        }
        double steering = PID.compute(getGyroError(target)) * timegain;
        Steering(0, steering);
        if (abs(getGyroError(target)) <= 1) {
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
}


void gyroPIDTurn_Task(void* pvParameters) {
    PIDTurnParameters *turnParameters = static_cast<PIDTurnParameters*>(pvParameters);
    gyroPIDTurn(turnParameters->target, turnParameters->timeout);
    vTaskDelete(NULL);
}



void gyroPIDForward(const int maxThrottle, const double target, const unsigned long timeForAccel, const unsigned long timeForConst) {
    int throttle = 0;
    int steering = 0;
    float diff;
    for (int i = 0; i < (timeForAccel / 10); i++) {
        throttle = (double)maxThrottle * ((double)i / ((double)timeForAccel / 10.0d));
        steering = PID.compute(getGyroError(target));
        Steering(throttle, steering);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    unsigned long start = millis();
    while (millis() - start <= timeForConst) {
        steering = PID.compute(getGyroError(target));
        Steering(maxThrottle, steering);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    for (int i = (timeForAccel / 10); i >= 0; i--) {
        throttle = (double)maxThrottle * ((double)i / ((double)timeForAccel / 10.0d));
        steering = PID.compute(getGyroError(target));
        Steering(throttle, steering);
        vTaskDelay(pdMS_TO_TICKS(10));                                                                                                                                                                              
    }
}


void AutomaticMove(void* pvParameters) {
    int *num = static_cast<int*>(pvParameters); 
    if (*num == 0) { //行き
        gyroPIDTurn(0.0, 3000);
        gyroPIDForward(60, 0.0, 200, 1600);
        gyroPIDTurn(90.0, 3000);
        gyroPIDForward(60, 90.0, 400, 1300);
        gyroPIDTurn(180.0, 3000);
        gyroPIDForward(60, 180.0, 500, 7000);
    }
    else if (*num == 1) { //帰り
        gyroPIDForward(60, 0.0, 500, 7000);
        gyroPIDTurn(270.0, 3000);
        gyroPIDForward(60, 270.0, 400, 1300);
        gyroPIDTurn(180.0, 3000);
        gyroPIDForward(60, 180.0, 200, 1600);
    }
    vTaskDelete(NULL);
}



void loop(void) {
    const char* TAG = "loop";


    if (isConnected) {
        enum ControlMode{Normal, GyroPIDTurn, LineTrace, Auto};
        static ControlMode mode = Normal;
        static bool isSwitched = true;
        static MotorData motorData_old;
        static uint8_t num_of_motor = 0;//////////////////////////////////////////////////////
        static int automatic_move_num = 0;

        if (nowConnected) {
            nowConnected = false;
            isSwitched = true;
            showConnection(isConnected, 1000);
            showServiceName(Main, Arm, LineTracer, 3000);
            num_of_motor = Main->getNumOfMotor();
            log_i("num_of_motor: %d", num_of_motor);
        }

        int throttle_power, steering_power;
        ButtonData buttonData = {.mode_select_pin = 0, 
                                 .up_pin = 0,
                                 .left_pin = 0,
                                 .down_pin = 0,
                                 .right_pin = 0,
                                 .joystick_pin = 0};
        constexpr uint8_t pins[6] = {UP_PIN, LEFT_PIN, DOWN_PIN, RIGHT_PIN, MODE_SELECT_PIN, JOYSTICK_BUTTON_PIN};
        static unsigned int count[6] = {0, 0, 0, 0, 0, 0};
        for (byte i = 0; i < 6; i++) {
            if (digitalRead(pins[i]) == LOW) {
                count[i]++;
                if (count[i] == 100) {
                    switch(i) {
                        case 0:
                            buttonData.up_pin = 2;
                            break;
                        case 1:
                            buttonData.left_pin = 2;
                            break;
                        case 2:
                            buttonData.down_pin = 2;
                            break;
                        case 3:
                            buttonData.right_pin = 2;
                            break;
                        case 4:
                            buttonData.mode_select_pin = 2;
                            break;
                        case 5:
                            buttonData.joystick_pin = 2;
                    }
                }
            } 
            else if (count[i] != 0) {
                if (count[i] < 100) {
                    switch(i) {
                        case 0:
                            buttonData.up_pin = 1;
                            break;
                        case 1:
                            buttonData.left_pin = 1;
                            break;
                        case 2:
                            buttonData.down_pin = 1;
                            break;
                        case 3:
                            buttonData.right_pin = 1;
                            break;
                        case 4:
                            buttonData.mode_select_pin = 1;
                            break;
                         case 5:
                            buttonData.joystick_pin = 1;

                    }
                }
                count[i] = 0;
            }
        }
        


        enum JoystickState{ELSE, UP, DOWN, LEFT, RIGHT};
        JoystickState jState, jMoment;
        static JoystickState jState_Old;
        int x = analogReadMilliVolts(JOYSTICK_X_PIN);
        int y = analogReadMilliVolts(JOYSTICK_Y_PIN);
        if (x <= 200) {
            jState = LEFT;
        } else if (x >= 3000) {
            jState = RIGHT;
        } else if (y <= 200) {
            jState = UP;
        } else if (y >= 3000) {
            jState = DOWN;
        } else {
            jState = ELSE;
        }
        if (jState != ELSE && jState != jState_Old) {
            jMoment = jState;
        } else {
            jMoment = ELSE;
        }
        jState_Old = jState;

        compass.draw(tft, bnoData.euler);
        switch (mode) {
            case Normal:
                if (isSwitched) {
                    tft.fillScreen(TFT_BLACK);
                    tft.setTextSize(4);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_WHITE);
                    tft.println("Normal");
                    compass.init(tft.width() / 2, tft.height() / 2, 90, TFT_GREEN);
                    compass.draw(tft, bnoData.euler, true);
                    isSwitched = false;
                }
                Command command;
                Main->getCommand(command);
                if (command.command == 0) { //通常状態
                    uint32_t Volt_Throttle = analogReadMilliVolts(THROTTLE_PIN);
                    uint32_t Volt_Steering = analogReadMilliVolts(STEERING_PIN);
                    switch(num_of_motor) {
                        case 2:
                            throttle_power = map(Volt_Throttle, 1670, 2200, -50, 50);
                            throttle_power = constrain(throttle_power, -50, 50);
                            if (abs(throttle_power) <= 10) {
                                throttle_power = 0;
                            } else if (throttle_power > 0) {
                                throttle_power -= 10;
                            } else {
                                throttle_power += 10;
                            }
                            steering_power = map(Volt_Steering, 1100, 2200, 50, -50);
                            steering_power = constrain(steering_power, -50, 50);
                            if (abs(steering_power) <= 10) {
                                steering_power = 0;
                            } else if (steering_power > 0) {
                                steering_power -= 10;
                            } else {
                                steering_power += 10;
                            }
                            motorData.power[0] = throttle_power + steering_power;
                            motorData.power[1] = throttle_power - steering_power;
                            break;
                        case 3:
                            break;
                        case 4:
                            break;
                    }
                    if (buttonData.mode_select_pin == 2) { //ジャイロゼロ点調整
                        command.command = 1;
                        command.parameter = 0;
                        Main->setCommand(command);
                    }
                } 
                else {
                    tft.setTextSize(2);
                    tft.fillRect(0, tft.height() - tft.fontHeight(), tft.width(), tft.fontHeight(), TFT_BLACK);
                    tft.setCursor(tft.width() - tft.textWidth("Executing command."), tft.height() - tft.fontHeight());
                    tft.print("Executing command.");
                }
                if (buttonData.up_pin == 1 || buttonData.left_pin == 1 || buttonData.down_pin == 1 || buttonData.right_pin == 1) { //４方向の旋回
                    isSwitched = true;
                    mode = GyroPIDTurn;
                    if (buttonData.up_pin == 1) { 
                        turnParameters.target = 0;
                    }
                    if (buttonData.right_pin == 1) {
                        turnParameters.target = 90;
                    }
                    if (buttonData.down_pin == 1) {
                        turnParameters.target = 180;
                    }
                    if (buttonData.left_pin == 1) { 
                        turnParameters.target = 270;
                    }
                }
                if (buttonData.up_pin == 2) {  //自律制御モード(往路)
                    isSwitched = true;
                    mode = Auto;
                    automatic_move_num = 0;
                }
                if (buttonData.down_pin == 2) { //自律制御モード(復路)
                    isSwitched = true;
                    mode = Auto;
                    automatic_move_num = 1;
                }
                if (jMoment != ELSE) {
                    Command command = {.command = 1, .parameter = 0};
                    switch(jMoment) {            
                        case UP:
                            command.parameter = 0;
                            break;
                        case RIGHT:
                            command.parameter = 1;
                            break;
                        case DOWN:
                            command.parameter = 2;
                            break;
                        case LEFT:
                            command.parameter = 3;
                            break;
                    }
                    Arm->setCommand(command);
                    ESP_LOGI("test", "send command to arm!!");
                }
                if (buttonData.joystick_pin == 1) {
                    Command command = {.command = 2, .parameter = 0};
                    Arm->setCommand(command);
                    ESP_LOGI("test", "send command to arm!!");
                }
                else if (buttonData.joystick_pin == 2) {
                    Command command = {.command = 3, .parameter = 0};
                    Arm->setCommand(command);
                    ESP_LOGI("test", "send command to arm!!");
                }
                break;

            case GyroPIDTurn: {
                static TaskHandle_t turn_handle;
                if (isSwitched) {
                    isSwitched = false;
                    PID.setParameters(parameters);
                    tft.fillScreen(TFT_BLACK);
                    tft.setTextSize(3);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_YELLOW);
                    tft.println("GyroPIDTurn"); 
                    compass.draw(tft, bnoData.euler, true);
                    compass.highLight(tft, turnParameters.target);
                    xTaskCreateUniversal(gyroPIDTurn_Task, "PIDTurn", 4096, &turnParameters, 0, &turn_handle, APP_CPU_NUM);
                }
                if (eTaskGetState(turn_handle) == eDeleted) {
                    mode = Normal;
                    isSwitched = true;
                }
                break;
            }

            case LineTrace:
                if (isSwitched) {
                    isSwitched = false;
                    tft.fillScreen(TFT_BLACK);
                    tft.setTextSize(3);
                    tft.setCursor(0, 0);
                    tft.setTextColor(TFT_WHITE);
                    tft.println("LineTrace");
                    compass.draw(tft, bnoData.euler, true);
                }
                if (buttonData.mode_select_pin == 1) {
                    mode = Normal;
                    isSwitched = true;
                    break;
                }
                break;

            case Auto:
                {
                    static TaskHandle_t auto_handle;
                    if (isSwitched) {
                        isSwitched = false;
                        tft.fillScreen(TFT_BLACK);
                        tft.setTextSize(4);
                        tft.setCursor(0, 0);
                        tft.setTextColor(TFT_WHITE);
                        tft.println("Auto");
                        xTaskCreateUniversal(AutomaticMove, "Auto", 4096, &automatic_move_num, 0, &auto_handle, APP_CPU_NUM);
                        compass.draw(tft, bnoData.euler, true);
                    }
                    if (buttonData.mode_select_pin == 1) {
                        vTaskDelete(auto_handle);
                    }
                    if (eTaskGetState(auto_handle) == eDeleted) {
                        mode = Normal;
                        isSwitched = true;
                    }
                }
                break;
        }
        if (motorData != motorData_old) {
            Main->setMotorData(motorData);
            motorData_old = motorData;
            showMotorData(motorData, num_of_motor, TFT_BLACK);
        }
    }



    else {
        showConnection(isConnected, 1000);
        Main->setCommand(command_null);
        Arm->setCommand(command_null);
        LineTracer->setCommand(command_null);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

