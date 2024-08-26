#include <Arduino.h>
#include <ESP32Servo.h>
Servo servo;

void setup(void) {
    servo.attach(13);
}

void loop(void) {
    servo.write(0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    servo.write(180);
    vTaskDelay(pdMS_TO_TICKS(1000));
}