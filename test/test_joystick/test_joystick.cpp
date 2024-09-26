#include<Arduino.h>

constexpr uint8_t JOYSTICK_BUTTON_PIN = 27;
constexpr uint8_t JOYSTICK_X_PIN = 35;
constexpr uint8_t JOYSTICK_Y_PIN = 34;


void setup(void) {
    Serial.begin(115200);
    pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(JOYSTICK_X_PIN, ANALOG);
    pinMode(JOYSTICK_Y_PIN, ANALOG);
}


enum JoystickState{ELSE, UP, DOWN, LEFT, RIGHT};
JoystickState jState;
void loop(void) {
    const char* TAG = "test";
    int x = analogReadMilliVolts(JOYSTICK_X_PIN);
    int y = analogReadMilliVolts(JOYSTICK_Y_PIN);
    if (x <= 200) {
        jState = LEFT;
        ESP_LOGI(TAG, "LEFT");
    } else if (x >= 3000) {
        jState = RIGHT;
        ESP_LOGI(TAG, "RIGHT");
    } else if (y <= 200) {
        jState = UP;
        ESP_LOGI(TAG, "UP");
    } else if (y >= 3000) {
        jState = DOWN;
        ESP_LOGI(TAG, "DOWN");
    } else {
        jState = ELSE;
        ESP_LOGI(TAG, "ELSE");
    }
    bool b = !digitalRead(JOYSTICK_BUTTON_PIN);
    if (b) {
        ESP_LOGI(TAG, "button is pressed.");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}


