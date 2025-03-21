#ifndef NEW_ELEVATOR
#define NEW_ELEVATOR

#include <tb6612fng.hpp>
constexpr int THRESHOLD = 1000;

class Elevator {
    public:
        Elevator(void) = delete;
        Elevator(const uint8_t& _pin1, const uint8_t& _pin2, const uint8_t& _pin_pwm, const uint8_t& _pin_stby, const unsigned long _time);
        bool rise(void);
        bool fall(void);
        int getState(void) const;

    private:
        const char* TAG;
        tb6612fng tb6612;
        const unsigned long time;        
        int state;
        friend void _rise(void* pvParameters);
        friend void _fall(void* pvParameters);
};

void _rise(void* pvParameters) {
    Elevator *elevator = static_cast<Elevator*>(pvParameters);
    elevator->state = 2;
    elevator->tb6612.A.drive(-255);
    vTaskDelay(pdMS_TO_TICKS(time));
    elevator->tb6612.A.drive(0);
    elevator->state = 1;
    vTaskDelete(NULL);
}

void _fall(void* pvParameters) {
    Elevator *elevator = static_cast<Elevator*>(pvParameters);
    elevator->state = 3;
    elevator->tb6612.A.drive(255);
    vTaskDelay(pdMS_TO_TICKS(time));
    elevator->tb6612.A.drive(0);
    elevator->state = 0;
    vTaskDelete(NULL);
}

Elevator::Elevator(const uint8_t& _pin1, const uint8_t& _pin2, const uint8_t& _pin_pwm, const uint8_t& _pin_stby, const unsigned long _time) 
    : TAG("elevator"), time(_time), state(0) {
    tb6612.setStandbyPin(_pin_stby);
    tb6612.on();
    tb6612.A.attach(_pin1, _pin2, _pin_pwm);
}

bool Elevator::rise(void) {
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


bool Elevator::fall(void) {
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

int Elevator::getState(void) const {
    return state;
}


# endif