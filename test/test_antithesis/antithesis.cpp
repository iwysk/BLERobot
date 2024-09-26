#include <Arduino.h>
#include <ESP32Servo.h>

//下で止まっている 0
//上で止まっている 1
//上昇中　2
//下降中　3

constexpr int THRESHOLD = 1000;

struct Command {
    public:
        uint8_t command;
        int parameter;
};

struct ArmData {
    public:
        short motorPower[2];
        short servo_angle[2];
        double sensor_value[2];
};


class Elevator_NoSensor {
    public:
        Elevator_NoSensor(void) = delete;
        Elevator_NoSensor(const uint8_t _in1_pin, const uint8_t _in2_pin, const unsigned long _time);
        bool rise(void);
        bool fall(void);
        int getState(void) const;

    private:
        const char* TAG;
        const uint8_t in1_pin;
        const uint8_t in2_pin;
        const unsigned long time;        
        int state;
        friend void _rise(void* pvParameters);
        friend void _fall(void* pvParameters);
};

void _rise(void* pvParameters) {
    Elevator_NoSensor *elevator = static_cast<Elevator_NoSensor*>(pvParameters);
    elevator->state = 2;
    digitalWrite(elevator->in1_pin, LOW);
    digitalWrite(elevator->in2_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(3100));
    digitalWrite(elevator->in1_pin, HIGH);
    digitalWrite(elevator->in2_pin, HIGH);
    elevator->state = 1;
    vTaskDelete(NULL);
}

void _fall(void* pvParameters) {
    Elevator_NoSensor *elevator = static_cast<Elevator_NoSensor*>(pvParameters);
    elevator->state = 3;
    digitalWrite(elevator->in1_pin, HIGH);
    digitalWrite(elevator->in2_pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(2500));
    digitalWrite(elevator->in1_pin, HIGH);
    digitalWrite(elevator->in2_pin, HIGH);
    elevator->state = 0;
    vTaskDelete(NULL);
}

Elevator_NoSensor::Elevator_NoSensor(const uint8_t _in1_pin, const uint8_t _in2_pin, const unsigned long _time) 
    : TAG("elevator"), in1_pin(_in1_pin), in2_pin(_in2_pin), time(_time), state(0) {
    pinMode(in1_pin, OUTPUT);
    pinMode(in2_pin, OUTPUT);
    digitalWrite(in1_pin, LOW);
    digitalWrite(in2_pin, LOW);
}

bool Elevator_NoSensor::rise(void) {
    if (state == 0) {
        xTaskCreateUniversal(_rise, "rise", 2048, this, 0, NULL, APP_CPU_NUM);
        return true;
    }
    if (state == 1) {
        ESP_LOGE(TAG, "Elevator is already in high position.");
    }
    else {
        ESP_LOGE(TAG, "Elevator is now moving. plase wait...");
    }
    return false;
}



bool Elevator_NoSensor::fall(void) {
    if (state == 1) {
        xTaskCreateUniversal(_fall, "fall", 2048, this, 0, NULL, APP_CPU_NUM);
        return true;
    }
    if (state == 0) {
        ESP_LOGE(TAG, "Elevator is already in low position.");
    }
    else {
        ESP_LOGE(TAG, "Elevator is now moving. plase wait...");
    }
    return false;
}

int Elevator_NoSensor::getState(void) const {
    return state;
}

constexpr uint8_t MOTOR_IN1_PIN = 22;
constexpr uint8_t MOTOR_IN2_PIN = 23;
constexpr uint8_t SERVO_ARM_PIN = 21;
constexpr uint8_t SERVO_ELEVETOR_PIN = 19;
constexpr uint8_t SERVO_LEFT_PIN = 18;
constexpr uint8_t SERVO_RIGHT_PIN = 5;
constexpr unsigned long movingtime = 2500;

Servo servo_arm, servo_elevetor, servo_gateL, servo_gateR;
constexpr uint8_t arm_laser_pin = 13;
constexpr uint8_t arm_ball_detection_pin = 14;
constexpr uint8_t elevetor_ball_detection_pin = 27;

Elevator_NoSensor *elevator;
uint8_t ball_count_L = 0;
uint8_t ball_count_R = 0;
uint8_t ball_count = 0;

const Command command_null = {.command = 0, .parameter = 0};
uint8_t mainUnitNum;
Command command;
ArmData armData;
ArmData armData_null = {.motorPower = {0, 0}, .servo_angle = {0, 0}, .sensor_value = {0, 0}};

#define END 0xC0
#define ESC 0xDB 
#define ESC_END 0xDC
#define ESC_ESC 0xDD

//mainUnitNum, command, armData, 0xC0
bool analyzeCommand(String packet, uint8_t& mainUnitNum, Command& command, ArmData& armData) {
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
    mainUnitNum = static_cast<uint8_t>(packet.charAt(0));
    packet.remove(0, 1);
    memcpy(&command, packet.substring(0, sizeof(Command) - 1).c_str(), sizeof(Command));
    packet.remove(0, sizeof(Command));
    if (sizeof(ArmData) != packet.length()) {
        ESP_LOGW(TAG, "size of data is incorrect.:%d", packet.length());
        return false;
    }
    memcpy(&armData, packet.c_str(), sizeof(ArmData));
    ESP_LOGI(TAG, "command: %d, %d", command.command, command.parameter);
    return true;
}



void receive_task(void* pvParameters) {
    const char* TAG = "Recieve";
    Serial1.setTimeout(100);
    while (true) {
        String packet = Serial1.readStringUntil(END);
        if (packet != NULL) {
            if (analyzeCommand(packet, mainUnitNum, command, armData)) {
                ESP_LOGI(TAG, "Successed to analyze command");
            }
        }
        vTaskDelay(50);
    }
}


void sendToMainUnit(const ArmData armData, const uint8_t ball_count) {
    uint8_t packet[256];
    packet[0] = 0x01;
    uint8_t data1[sizeof(ArmData)];
    memcpy(data1, &armData, sizeof(ArmData));
    for (int i = 0; i < sizeof(ArmData); i++) {
        packet[i + 1] = data1[i];
    } 
    packet[sizeof(ArmData) + 1] = ball_count;
    for (int i = 0; i < 1 + sizeof(ArmData) + 1; i++) {
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


inline void ball_catch(const int begin_angle) {
    servo_arm.write(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    servo_arm.write(begin_angle);
}


inline void ball_release(const bool isLeft) {
    if (isLeft) {
        servo_gateL.write(130);
        vTaskDelay(600);
        servo_gateL.write(30);
        vTaskDelay(600);
        if (ball_count_L > 0) {
            ball_count_L--;
            ball_count = ball_count_L + ball_count_R;
            ESP_LOGI("test", "sendMessage");
            sendToMainUnit(armData_null, ball_count);
        }
    }
    else {
        servo_gateR.write(50);
        vTaskDelay(600);
        servo_gateR.write(150);
        vTaskDelay(600);
        if (ball_count_R > 0) {
            ball_count_R--;
            ball_count = ball_count_L + ball_count_R;
            ESP_LOGI("test", "sendMessage");
            sendToMainUnit(armData_null, ball_count);
        }
    }
}



void elevator_task(void* pvParameters) {
    const char* TAG = "Elevator";
    while (true) {
        if (0 == elevator->getState()) {
            if (analogRead(elevetor_ball_detection_pin) >= THRESHOLD) {
                ESP_LOGI(TAG, "Detected ball.");
                if (ball_count == 6) {
                    ESP_LOGW(TAG, "Ball tank is already filled.");
                }
                else {
                    if (ball_count_R < 3) {
                        servo_elevetor.write(90 + 38);
                    }
                    else {  //ball_count_L < 3 && ball_count_R == 3
                        servo_elevetor.write(90 - 38);
                    }
                    elevator->rise();
                }
            }
        }
        else if (1 == elevator->getState()) {
            if (ball_count_R < 3) {
                servo_elevetor.write(90 - 38);
                ball_count_R++;
            }
            else {
                servo_elevetor.write(90 + 38);
                ball_count_L++;
            }
            ball_count = ball_count_L + ball_count_R;
            sendToMainUnit(armData_null, ball_count);
            ESP_LOGI(TAG, "Ball Tank Usage:%d, R:%d, total:%d", ball_count_L, ball_count_R, ball_count);
            vTaskDelay(pdMS_TO_TICKS(300));
            elevator->fall();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



void setup(void) {
    const char* TAG = "init";
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial1.setPins(33, 32);
    ESP_LOGI(TAG, "ARM UNIT 01: ANTITHESIS");

    pinMode(arm_laser_pin, OUTPUT);
    digitalWrite(arm_laser_pin, HIGH);
    pinMode(arm_ball_detection_pin, ANALOG);
    pinMode(elevetor_ball_detection_pin, ANALOG);
    
    servo_arm.attach(SERVO_ARM_PIN);
    servo_arm.write(0);
    servo_elevetor.attach(SERVO_ELEVETOR_PIN);
    servo_gateL.attach(SERVO_LEFT_PIN);
    servo_gateR.attach(SERVO_RIGHT_PIN);
    servo_gateL.write(150);
    servo_gateR.write(30);
    elevator = new Elevator_NoSensor(MOTOR_IN1_PIN, MOTOR_IN2_PIN, movingtime);
    sendToMainUnit(armData_null, 0);    //コネクション確立用に最初の一回だけ送信
    xTaskCreateUniversal(elevator_task, "elevetor", 2048, NULL, 0, NULL, APP_CPU_NUM);
    xTaskCreateUniversal(receive_task, "command", 2048, NULL, 0, NULL, APP_CPU_NUM);
}




enum Mode_t {WAITING, CATCH, RELEASE};

void loop(void) 
{ //アームの方
    const char* TAG = "ARM"; 
    static Mode_t mode = WAITING;
    static int begin_angle = 180;

    switch (mode) {
        case WAITING:
            if (command.command == 3) {
                mode = CATCH;
                servo_arm.write(180);
                servo_gateL.write(30);
                servo_gateR.write(150);
            }
            break;

        case CATCH:
            if (analogRead(arm_ball_detection_pin) >= THRESHOLD) {
                ESP_LOGI(TAG, "arm detected ball.");
                ball_catch(begin_angle);
            }
            if (command.command == 1) {
                if (command.parameter == 0) {
                    if (begin_angle <= 170) {
                        begin_angle += 10;
                    }
                    servo_arm.write(begin_angle);
                }
                else if (command.parameter == 2) {
                    if (begin_angle >= 10) {
                        begin_angle -= 10;
                    }
                    servo_arm.write(begin_angle);
                }
            }
            if (command.command == 2) {
                ball_catch(begin_angle);
            }
            if (command.command == 3) {
                mode = RELEASE;
                servo_arm.write(0);
                digitalWrite(arm_laser_pin, LOW);
            }
            break;

        case RELEASE:
            if (command.command == 1) {
                if (command.parameter == 1) {
                    ball_release(false);
                }
                else if (command.parameter == 3) {
                    ball_release(true);
                }
            }
            if (command.command == 3) {
                mode = CATCH;
                servo_arm.write(180);
                digitalWrite(arm_laser_pin, HIGH);
            }
            break;
    }
    command = command_null;
    vTaskDelay(pdMS_TO_TICKS(10));
}


