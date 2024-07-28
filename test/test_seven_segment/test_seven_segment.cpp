#include <Arduino.h>
#include "seven_segment.hpp"

SevenSegment* seg = nullptr;

void setup(void) {
    Serial.begin(115200);
    seg = new SevenSegment(19, 21, 22, 13, 12, 14, 27, 26);
    for (int i = 0; i <= 9; i++) {  
        seg->oneNumber(i, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void loop(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
}