#include <Arduino.h>
#include <seven_segment.hpp>

uint8_t pins[7] = {5, 12, 13, 14, 16, 17, 21};
SevenSegment *seg;
void setup(void) {
    // seg = new SevenSegment(5, 12, 13, 14, 16, 17, 21); //なんか19番ピンだと動かない←TFTのMISOだった
    // seg->Number(0);
     for (int i = 0; i < 7; i++) {
        pinMode(pins[i], OUTPUT);
    }
}


void loop(void) {
    for (int i = 0; i < 7; i++) {
        digitalWrite(pins[i], HIGH);
        delay(1000);
    }
}