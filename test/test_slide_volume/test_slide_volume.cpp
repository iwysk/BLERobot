#include <Arduino.h>
#include <esp_log.h>
#include <driver/adc.h>
#include <esp_efuse.h>
#include <esp_adc_cal.h>

gpio_num_t analog_pin;
esp_adc_cal_characteristics_t adcChar;

void setup(void) 
{
    Serial.begin(115200);
    static const adc_unit_t unit = ADC_UNIT_1;
    static const adc1_channel_t channel = ADC1_CHANNEL_6;
    adc1_pad_get_io_num(channel, &analog_pin);
    log_i("analog_pin: %d ", analog_pin);
    // 11dB減衰を設定。フルスケールレンジは3.9V
    // SENSOR_VPピンにはバッテリ電圧の1/2 (約1.85V）が印加される
    //  6dBで正確な値が取れる電圧範囲は0.15 ~ 1.75V
    //  11dBで正確な値が取れる電圧範囲は0.15 ~ 2.45V
    // よって、11dB減衰を設定する
    static const adc_atten_t        atten = ADC_ATTEN_DB_11;
    // ADCの分解能を12bit (0~4095)に設定
    static const adc_bits_width_t   width = ADC_WIDTH_BIT_12;
 
    // ADCのユニットとチャンネルの設定
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    // ADC1の解像度を12bit（0~4095）に設定
    adc1_config_width(width);
    // ADC1の減衰を11dBに設定
    adc1_config_channel_atten(ADC1_CHANNEL_6, atten);
    // 電圧値に変換するための情報をaddCharに格納
    esp_adc_cal_characterize(ADC_UNIT_1, atten, width, 1100, &adcChar);
}

void loop(void) {
    int value = analogRead(analog_pin);
//    int value_raw = analogReadRaw(analog_pin);
//    log_i("pin:%d value:%d value_raw%d", analog_pin, value, value_raw);
    uint32_t voltage, voltage_cal;
    voltage = analogReadMilliVolts(analog_pin);
    esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adcChar, &voltage_cal);
    log_i("voltage: %ld, voltage_cal: %ld", voltage, voltage_cal);
    vTaskDelay(pdMS_TO_TICKS(100));
}