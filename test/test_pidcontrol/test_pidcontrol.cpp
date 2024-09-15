#include <Arduino.h>
#include <pidcontrol.hpp>

PIDParameters parameters = {.pGain = 1, .iGain = 0.0001, .dGain = 0};
PIDControl *PID;

void setup(void) {
    Serial.begin(115200);
    PID = new PIDControl(parameters);
    double target[4] = {90, 180, -90, 60};
    int count = 0;
    double angle = 0;
    for (int i = 0; i < 4; i++) {
        while (1) {
            Serial.println(angle);
            double throttle = PID->compute(angle - target[i]);
            angle -= throttle * 0.01;
            if (abs(angle - target[i]) <= 1) {
                count++;
                if (count >= 50) {
                    Serial.println("Successed to reach target.");
                    break;
                }
            }
            else {
                count = 0;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        PID->reset();
    }
}


void loop(void) {         
    vTaskDelay(pdMS_TO_TICKS(100));
}
