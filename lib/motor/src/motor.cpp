#include <motor.hpp>

Motor::Motor(void) : TAG("Motor"), isAttached(false) {
    ESP_LOGI(TAG, "constructor");
}

void Motor::attach(const uint8_t& pin_dir, const uint8_t& pin_pwm) {
    this->pin_dir = pin_dir;
    this->pin_pwm = pin_pwm;
    isAttached = true;
    ESP_LOGI(TAG, "attach() pin_dir: %d, pin_pwm:%d", this->pin_dir, this->pin_pwm);
    pinMode(this->pin_dir, OUTPUT);
    pinMode(this->pin_pwm, OUTPUT);
}

void Motor::detach(void) {
    if (isAttached) {
        pinMode(this->pin_dir, INPUT);
        pinMode(this->pin_pwm, INPUT);
        ESP_LOGI(TAG, "Successfully detached motor.");
    }
    else {
        ESP_LOGI(TAG, "Motor::attach() has not called yet or motor is already detached.");
    }
}

Motor::~Motor(void) {
    ESP_LOGI(TAG, "destractor");
}

void Motor::drive(const int &power) {
    bool direction = power >= 0;
    ESP_LOGD(TAG, "direction: %d, |power| = %d", direction, abs(power));
    if (direction) {
        digitalWrite(pin_dir, HIGH);
    }
    else {
        digitalWrite(pin_dir, LOW);
    }
    analogWrite(pin_pwm, abs(power));
}