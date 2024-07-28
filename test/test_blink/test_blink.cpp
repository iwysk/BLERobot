#include <Arduino.h>

constexpr uint8_t ledPin[2] = {32, 19};

void blink(void* pvParamaters) {
    pinMode(ledPin[1], OUTPUT);
    while (1) {
        digitalWrite(ledPin[1], HIGH);
        delay(200);
        digitalWrite(ledPin[1], LOW);
        delay(200);
    }
}


void setup(void) {
    pinMode(ledPin[0], OUTPUT);
    xTaskCreateUniversal(blink, "blink", 1024, nullptr, 0, nullptr, APP_CPU_NUM);
}

void loop(void) {
    digitalWrite(ledPin[0], HIGH);
    delay(500);
    digitalWrite(ledPin[0], LOW);
    delay(500);
}

// static const char* TAG = "dac_audio";
// class dacAudio { //monoral only
//     public:
//         dacAudio(void);
//         static void play(void);
//         static void stop(void);
//     private:
//         ;
// };

// dacAudio::dacAudio(void) {
//     ;
// }

// void dacAudio::play(void) {
//     ;
// }

// void dacAudio::stop(void) {
//     ;
// }

// //22050Hz

// extern "C" void app_main(void)
// {
//     dac_continuous_handle_t dacHandle;
//     dac_continuous_config_t dacConfig = {.chan_mask = DAC_CHANNEL_MASK_CH0,
//                                          .desc_num = 50,
//                                          .buf_size = 4092,
//                                          .freq_hz = 48000,
//                                          .offset = 0,
//                                          .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
//                                          .chan_mode = DAC_CHANNEL_MODE_SIMUL
//                                         }; //2046
//     ESP_ERROR_CHECK(dac_continuous_new_channels(&dacConfig, &dacHandle));
//     ESP_ERROR_CHECK(dac_continuous_enable(dacHandle));
//     size_t bytes_loaded = 0;
//     ESP_LOGI(TAG, "start playing!!");
//     ESP_ERROR_CHECK(dac_continuous_write_cyclically(dacHandle, (uint8_t*)rawData, sizeof(rawData), &bytes_loaded)); //DMAにバイトがロードされるとreturnする
//     ESP_LOGI(TAG, "bytes_loaded: %d", bytes_loaded);
//     vTaskDelay(pdMS_TO_TICKS(2000));
//     dac_continuous_disable(dacHandle);
// }