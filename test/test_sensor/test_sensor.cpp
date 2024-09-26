#include <Arduino.h>


void setup(void) {
    Serial.begin(115200);
    pinMode(14, ANALOG);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
}


void loop(void) {
    Serial.println(analogRead(14));
    vTaskDelay(pdMS_TO_TICKS(100));
}