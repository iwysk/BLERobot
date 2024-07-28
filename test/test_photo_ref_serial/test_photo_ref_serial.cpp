#include <Arduino.h>



void serial1_send(void* pvParameters) {
    while (1) {
        Serial1.println("ABCDEFGHIJKLMN");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void serial2_send(void* pvParameters) {
    while (1) {
        Serial2.println("abcde");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


void setup(void) {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 33, 32);
    Serial2.begin(9600, SERIAL_8N1, 18, 19);
    xTaskCreateUniversal(serial1_send, "serial1_send", 1024, nullptr, 0, nullptr, APP_CPU_NUM);
    xTaskCreateUniversal(serial2_send, "serial2_send", 1024, nullptr, 0, nullptr, APP_CPU_NUM);
}


void loop(void) {
    if (Serial2.available() > 0) {
        char c = Serial2.read();
        Serial.print(c);
    }
    if (Serial1.available() > 0) {
        char c = Serial1.read();
        Serial.print(c);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}