#ifndef TB6612FNG
#define TB6612FNG

#include <Arduino.h>
class SingleMotor final {
    public:
        SingleMotor(void);
        ~SingleMotor(void);
        void attach(const uint8_t& _pin1, const uint8_t& _pin2,  const uint8_t& _pin_pwm);
        void detach(void);
        void drive(const int &power);
    private:
        const char* TAG;
        bool isAttached;
        uint8_t pin1, pin2, pin_pwm;
};


class tb6612fng {
    public: 
        void setStandbyPin(const uint8_t& _stby_pin);
        void on(void);
        void off(void);
        SingleMotor A, B;
    private:
        uint8_t stby_pin;
};


#endif
