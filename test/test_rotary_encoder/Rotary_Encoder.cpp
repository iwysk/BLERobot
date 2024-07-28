#include <Arduino.h>
#include <driver/pcnt.h>

const int pulse_pin_clk = 25;
const int pulse_pin_ctl = 26;

void setup(void) {
    Serial.begin(115200);
    pinMode(pulse_pin_clk, INPUT_PULLDOWN);
    pinMode(pulse_pin_ctl, INPUT_PULLDOWN);    
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = pulse_pin_clk,
        .ctrl_gpio_num = pulse_pin_ctl,
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_REVERSE,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .counter_h_lim = 32767,
        .counter_l_lim = -32768,
        .unit = PCNT_UNIT_0,
        .channel = PCNT_CHANNEL_0
    };
    pcnt_unit_config(&pcnt_config);
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
    
    pinMode(pulse_pin_clk, INPUT_PULLDOWN);
    pinMode(pulse_pin_ctl, INPUT_PULLDOWN);    
}

void loop(void) {
    short int count = 0;
    double angle;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);
    angle = (double)count * (PI / 12.0);
    Serial.printf(">counter:%d >angle:%f\r\n", count, angle);
    vTaskDelay(pdMS_TO_TICKS(10));
}