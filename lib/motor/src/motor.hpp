#ifndef MOTOR
#define MOTOR
#include <Arduino.h>

class Motor final {
    public:
        Motor(void);
        ~Motor(void);
        void attach(const uint8_t& pin_dir, const uint8_t& pin_pwm);
        void detach(void);
        void drive(const int &power);
    private:
        const char* TAG;
        bool isAttached;
        uint8_t pin_dir, pin_pwm;
};


#endif
