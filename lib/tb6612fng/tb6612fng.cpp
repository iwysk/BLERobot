#include <tb6612fng.hpp>


SingleMotor::SingleMotor(void) : TAG("Motor"), isAttached(false) {
    ESP_LOGI(TAG, "constructor");
}

void SingleMotor::attach(const uint8_t& _pin1, const uint8_t& _pin2,  const uint8_t& _pin_pwm) {
    this->pin1 = _pin1;
    this->pin2 = _pin2;
    this->pin_pwm = _pin_pwm;
    isAttached = true;
    ESP_LOGI(TAG, "attach() pin1: %d, pin2:%d pin_pwm:%d", this->pin1, this->pin2, this->pin_pwm);
    pinMode(this->pin1, OUTPUT);
    pinMode(this->pin2, OUTPUT);
    pinMode(this->pin_pwm, OUTPUT);
}

void SingleMotor::detach(void) {
    if (isAttached) {
        pinMode(this->pin1, INPUT);
        pinMode(this->pin2, INPUT);
        pinMode(this->pin_pwm, INPUT);
        ESP_LOGI(TAG, "Successfully detached motor.");
    }
    else {
        ESP_LOGI(TAG, "SingleMotor::attach() has not called yet or motor is already detached.");
    }
}

SingleMotor::~SingleMotor(void) {
    ESP_LOGI(TAG, "destractor");
}

void SingleMotor::drive(const int &power) {
    if (abs(power) <= 10) {
        ESP_LOGD(TAG, "brake");
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, HIGH);
        digitalWrite(pin_pwm, LOW);
    }
    else {
        bool direction = power >= 0;
        ESP_LOGD(TAG, "direction: %d, |power| = %d", direction, abs(power));
        if (direction) {
            digitalWrite(pin1, HIGH);
            digitalWrite(pin2, LOW);
        }
        else {
            digitalWrite(pin1, LOW);
            digitalWrite(pin2, HIGH);
        }
        analogWrite(pin_pwm, abs(power));
    }
}


void tb6612fng::setStandbyPin(const uint8_t& _stby_pin) {
    this->stby_pin = _stby_pin;
    pinMode(stby_pin, OUTPUT);
    digitalWrite(stby_pin, LOW);
}


void tb6612fng::on(void) {
    digitalWrite(stby_pin, HIGH);
}

void tb6612fng::off(void) {
    digitalWrite(stby_pin, LOW);
}