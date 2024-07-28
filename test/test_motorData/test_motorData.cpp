#include <Arduino.h>
#include <main_unit_manager.hpp>

// MotorData motorData1 = {.num_of_motor = 4, 
//                         .power[0] = 0, 
//                         .power[1] = 0, 
//                         .power[2] = 0, 
//                         .power[3] = 0, 
//                         .power[4] = 0};
// MotorData motorData2 = {.num_of_motor = 4,
//                         .power[0] = 0,
//                         .power[1] = 0,
//                         .power[2] = 0,
//                         .power[3] = 0,
//                         .power[4] = 0};
MotorData motorData1, motorData2;

void setup(void) {
    Serial.begin(115200);  
    bool result = motorData1 == motorData2;
    log_i("result: %d", result); 

    motorData1.num_of_motor = 2;
    result = motorData1 == motorData2;
    log_i("result: %d", result); 

    motorData1.power[0] = 100;
    motorData1.power[1] = -100;
    result = motorData1 == motorData2;
    log_i("result: %d", result); 

    motorData2.num_of_motor = 2;
    motorData2.power[0] = 100;
    motorData2.power[1] = -100;
    result = motorData1 == motorData2;
    log_i("result: %d", result); 

}
void loop(void) {

}