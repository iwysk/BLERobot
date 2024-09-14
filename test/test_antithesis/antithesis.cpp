#include <Arduino.h>
#include <ESP32Servo.h>

//下で止まっている 0
//上で止まっている 1
//上昇中　2
//下降中　3

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
    vTaskDelay(pdMS_TO_TICKS(elevator->time));
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
    vTaskDelay(pdMS_TO_TICKS(elevator->time));
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

int Elevator_NoSensor::getState(void) const  {
    return state;
}


constexpr uint8_t MOTOR_IN1_PIN = 21;
constexpr uint8_t MOTOR_IN2_PIN = 19;
constexpr unsigned long movingtime = 2500;

Servo servo_arm, servo_elevetor, servo_gate1, servo_gate2;
constexpr uint8_t arm_laser_pin = 32;
constexpr uint8_t arm_ball_detection_pin = 34;
constexpr uint8_t elevetor_ball_detection_pin = 35;

enum Mode_t{COLLECT, RELEASE, SLEEP};
Elevator_NoSensor *elevator;
int ball_count_L = 0;
int ball_count_R = 0;
int ball_count = 0;


void elevator_task(void* pvParameters) {
    constexpr int THRESHOLD = 1000;
    const char* TAG = "Elevator";
    while (true) {
        if (0 == elevator->getState()) {
            if (analogRead(elevetor_ball_detection_pin) <= THRESHOLD) {
                ESP_LOGI(TAG, "Detected ball.");
                if (ball_count == 8) {
                    ESP_LOGW(TAG, "Ball tank is already filled.");
                }
                else {
                    elevator->rise();
                }
            }
        }
        else if (1 == elevator->getState()) {
            if (ball_count_R <= 3) {
                servo_elevetor.write(90 + 40);
                ball_count_R++;
            }
            else {
                servo_elevetor.write(90 - 40);
                ball_count_L++;
            }
            ball_count = ball_count_L + ball_count_R;
            ESP_LOGI(TAG, "Ball Tank Usage:%d, R:%d, total:%d", ball_count_L, ball_count_R, ball_count);
            vTaskDelay(pdMS_TO_TICKS(300));
            servo_elevetor.write(90);
            elevator->fall();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



void command_receive_task(void* pvParameters) {
    const char* TAG = "Recieve";
    Serial.setTimeout(10);
    while (true) {
        String packet = Serial.readStringUntil('\n');
        if (packet != NULL) {
            char packet_c_str[256];
            packet.toCharArray(packet_c_str, 256);
            ESP_LOGI(TAG, "Received Command :%s", packet_c_str);
        }
        vTaskDelay(100);
    }
}


#define END 0xC0
#define ESC 0xDB 
#define ESC_END 0xDC
#define ESC_ESC 0xDD

//mainUnitNum, command, armData
bool analyzeCommand(String packet, uint8_t& mainUnitNum, Command& command, ArmData& armData) {
    if (packet.end() != END) {
        ESP_LOGW("Packet does not end with 0xC0.");
        return false;
    }
    packet.remove(packet.length() - 1); //末端を削除
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
    mainUnitNum = static_cast<uint8_t>(packet.begin());
    packet.remove(0, 1);
    memcpy(&command, packet.substring(0, sizeof(Command) - 1).c_str(), sizeof(Command));
    packet.remove(0, sizeof(Command));
    if (sizeof(ArmData) != packet.length() - 1) {
        ESP_LOGW("size of data is incorrect.");
        return false;
    }
    memcpy(&armData, packet.c_str(), sizeof(ArmData));
    ESP_LOGI(TAG, "command: %d, %d", command.command, command.parameter);
    return true;
}

void setup(void) {
    const char* TAG = "init";
    Serial.begin(115200);
    ESP_LOGI(TAG, "ARM UNIT 01: ANTITHESIS");
    pinMode(arm_laser_pin, OUTPUT);
    pinMode(arm_ball_detection_pin, ANALOG);
    pinMode(elevetor_ball_detection_pin, ANALOG);
    servo_arm.attach(12);
    servo_elevetor.attach(14);
    servo_gate1.attach(27);
    servo_gate2.attach(26);
    elevator = new Elevator_NoSensor(MOTOR_IN1_PIN, MOTOR_IN2_PIN, movingtime);
    xTaskCreateUniversal(elevator_task, "transport", 2048, NULL, 0, NULL, APP_CPU_NUM);
    xTaskCreateUniversal(command_receive_task, "command", 2048, NULL, 0, NULL, APP_CPU_NUM);
}



void loop(void) { //アームの方
    const char* TAG = "ARM"; 
    constexpr int THRESHOLD = 1000;
    static Mode_t mode = COLLECT;
    switch (mode) {
        case COLLECT:
            digitalWrite(arm_laser_pin, HIGH);
            if (analogRead(arm_ball_detection_pin) <= THRESHOLD) {
                ESP_LOGI(TAG, "arm detected ball.");
                servo_arm.write(180);
                vTaskDelay(pdMS_TO_TICKS(200));
                servo_arm.write(0);
            }
            break;
        case RELEASE:
            digitalWrite(arm_laser_pin, LOW);
            servo_arm.write(180);
            break;
        case SLEEP:
            break;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}


