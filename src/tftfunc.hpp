#ifndef TFTFUNC
#define TFTFUNC

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PNGDec.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
//PNGdec reference
//https://github.com/bitbank2/PNGdec/wiki

File pngFile;
PNG png;
uint16_t x, y;
const char *machine_name = "BIGBEAR";
#define MAX_WIDTH (320)

struct place {
    uint16_t x, y;
};

//PNG_OPEN_CALLBACK
void* SPIFFSOpen(const char* szFileName, int32_t *pFileSize) {
    pngFile = SPIFFS.open(szFileName, "r", false);
    *pFileSize = pngFile.size();
    return &pngFile;
}

//PNG_CLOSE_CALLBACK
void SPIFFSClose(void* pHandle) {
    pngFile.close();
}

//PNG_READ_CALLBACK 
int32_t SPIFFSRead(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    if (!pngFile) return 0;
    return pngFile.read(pBuf, iLen);
}

//PNG_SEEK_CALLBACK
int32_t SPIFFSSeek(PNGFILE *pFile, int32_t iPosition) {
    if (!pngFile) return 0;
    return pngFile.seek(iPosition);
}

//PNG_DRAW_CALLBACK
void funcPNGDraw(PNGDRAW *pDraw) {
    uint16_t pixels[MAX_WIDTH];
    png.getLineAsRGB565(pDraw, pixels, PNG_RGB565_BIG_ENDIAN, -1);
    struct place *png_place = static_cast<struct place*>(pDraw->pUser);
    tft.pushImage(png_place->x, png_place->y + pDraw->y, pDraw->iWidth, 1, pixels);
}

void showRogo(void) 
{
    const char* TAG = "Rogo";
    if (!SPIFFS.begin(true)) {
        ESP_LOGW(TAG, "A error occured during SPIFFS.begin()");
        return;
    }
    File rogo_file = SPIFFS.open("/rogo.png", "r", false);
    if (!rogo_file) {
        ESP_LOGI(TAG, "rogo file was not found.");
    }
    else {
        rogo_file.close();
        if (PNG_SUCCESS == png.open("/rogo.png", SPIFFSOpen, SPIFFSClose, SPIFFSRead, SPIFFSSeek, funcPNGDraw)) {
            ESP_LOGI(TAG, "Image size width: %d, height: $d, %d bpp, pixeltype: %d", png.getWidth(), png.getPixelType());
            struct place png_place;
            png_place.x = (tft.width() - png.getWidth()) / 2;
            png_place.y = (tft.height() - png.getHeight()) / 2;
            vTaskDelay(pdMS_TO_TICKS(1000));
            png.decode(&png_place, PNG_CHECK_CRC);
            png.close();
            vTaskDelay(pdMS_TO_TICKS(2000));
        } 
        else {
            ESP_LOGI(TAG, "Failed to decode rogo file");
        }
    }
}


void initTFT(void) 
{
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(3);
    showRogo();
    const char* official_machine_name[strlen(machine_name)] = {
                           "BLE", 
                           "Intaractive",
                           "General-purpose", 
                           "Bilateral arms",
                           "Expandable",
                           "Assalt",
                           "Robot"};
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(5);
    tft.setTextColor(TFT_SKYBLUE);
    tft.setCursor((tft.width() - tft.textWidth(machine_name)) / 2, 0); 
    tft.print(machine_name);  

    uint16_t y = tft.fontHeight();
    tft.setTextSize(3);
    for (uint8_t i = 0; i < strlen(machine_name); i++) {
        tft.setCursor(0, y);
        tft.setTextColor(TFT_SKYBLUE);
        tft.print(official_machine_name[i][0]);
        tft.setTextColor(TFT_MAGENTA);
        for (int j = 1; j < strlen(official_machine_name[i]); j++) {
            tft.print(official_machine_name[i][j]);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        y += tft.fontHeight();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    tft.setCursor(tft.width() - tft.textWidth("for SSR"), tft.height() - tft.fontHeight());
    tft.print("for SSR");
    vTaskDelay(pdMS_TO_TICKS(1000));
    tft.fillScreen(TFT_BLACK);
}


void showConnection(const bool isConnected, const unsigned long& time) {
    tft.fillScreen(TFT_BLACK);
    if (isConnected) {
        tft.setTextSize(5);
        tft.setCursor(0, (tft.height() - tft.fontHeight()) / 2);
        tft.setTextColor(TFT_GREEN);
        tft.print("Connected.");
    }
    else {
        tft.setTextSize(4);
        tft.setCursor(0, (tft.height() - tft.fontHeight()) / 2);
        tft.setTextColor(TFT_RED);
        tft.print("Disconnected.");
    }
    vTaskDelay(pdMS_TO_TICKS(time));
    tft.setTextSize(3);
    tft.fillScreen(TFT_BLACK);
    
}

void printTaskState(TaskHandle_t& handle) {
    eTaskState taskstate = eTaskGetState(handle);
    switch(taskstate) {
        case eRunning:          // 実行中
            log_i("runnning");
            break;
        case eReady:            // 実行可能（実行待ち）
            log_i("ready");
            break;
        case eBlocked:          // ブロックされている
            log_i("block");
            break;
        case eSuspended:        // 中断中
            log_i("suspended");
            break;
        case eDeleted:          // 削除済み
            log_i("deleted");
            break;
        case eInvalid:
            log_i("eroor: invalid");
            break;  
    }
}

void showServiceName(MainService* Main, ArmService* Arm, LineTracerService* LineTracer, const unsigned long& time) {
    const char* TAG = "LCDFunc";
    ESP_LOGI(TAG, "Show unit state");
    tft.setCursor(0, 0);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.println("Main Unit: "); 
    if (Main->isActivated()) {
        tft.setTextColor(TFT_GREEN); 
        tft.println(Main->getName().c_str());
    } 
    tft.println();

    tft.setTextColor(TFT_WHITE);
    tft.println("Arm Unit: "); 
    if (Arm->isActivated()) {
        tft.setTextColor(TFT_GREEN); 
        tft.println(Arm->getName().c_str());
    } 
    tft.println();

    tft.setTextColor(TFT_WHITE);
    tft.println("Linetrace Unit: ");
    if (LineTracer->isActivated()) {
        tft.setTextColor(TFT_GREEN); 
        tft.print(LineTracer->getName().c_str());
    }
    tft.println();
    vTaskDelay(pdMS_TO_TICKS(time));
    tft.fillScreen(TFT_BLACK);
}


class GyroCompass final {
    public:
        GyroCompass(void);
        void init(const uint32_t& _center_x, const uint32_t& _center_y, const uint32_t& _radius, const uint32_t& _frame_color);
        void drawFrame(TFT_eSPI& tft);
        void draw(TFT_eSPI& tft, const float* vector, const bool& drawFrame = false);
        void
    private:
        void cleanup(void);
        bool bInitialized;
        uint32_t center_x, center_y;
        uint32_t radius;
        uint32_t frame_color;
        float vector_old[3];
        static const uint32_t base_length;
        const char TAG[12];
};


const uint32_t GyroCompass::base_length = 30;

GyroCompass::GyroCompass(void) : TAG("Compass") {
    ESP_LOGD(TAG, "constructor");
    esp_log_level_set(TAG, ESP_LOG_INFO);
}

void GyroCompass::init(const uint32_t& _center_x, const uint32_t& _center_y, const uint32_t& _radius, const uint32_t& _frame_color) {
    bInitialized = true;
    center_x = _center_x;
    center_y = _center_y;
    radius = _radius;
    frame_color = _frame_color;
}

void GyroCompass::drawFrame(TFT_eSPI &tft) {
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Not initialized");
        return;
    }
    tft.setOrigin(center_x, center_y);
    tft.drawSmoothCircle(0, 0, radius, TFT_WHITE, frame_color);
    tft.fillCircle(0, 0, radius - 2, TFT_BLACK);
    tft.drawFastVLine(0, -radius, radius*2, frame_color);
    tft.drawFastHLine(-radius, 0, radius*2, frame_color);
    tft.drawLine((float)radius*cos(PI*3/4), (float)radius*sin(PI*3/4), (float)radius*cos(PI*7/4), (float)radius*sin(PI*7/4), frame_color);
    tft.drawLine((float)radius*cos(PI/4), (float)radius*sin(PI/4), (float)radius*cos(PI*5/4), (float)radius*sin(PI*5/4), frame_color);
    tft.setOrigin(0, 0);
}

void GyroCompass::cleanup(void) {
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Not initialized");
        return;
    }
    tft.setOrigin(center_x, center_y);
    tft.fillCircle(0, 0, radius -2, TFT_BLACK);
    tft.drawFastVLine(0, -radius, radius*2, frame_color);
    tft.drawFastHLine(-radius, 0, radius*2, frame_color);
    tft.drawLine((float)radius*cos(PI*3/4), (float)radius*sin(PI*3/4), (float)radius*cos(PI*7/4), (float)radius*sin(PI*7/4), frame_color);
    tft.drawLine((float)radius*cos(PI/4), (float)radius*sin(PI/4), (float)radius*cos(PI*5/4), (float)radius*sin(PI*5/4), frame_color);
    tft.setOrigin(0, 0);
}

void GyroCompass::draw(TFT_eSPI& tft, const float* vector, const bool& bDrawFrame) {
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Not initialized");
        return;
    }
    if (bDrawFrame) {
        drawFrame(tft);
    }
    if (vector_old[0] == vector[0] && vector_old[1] == vector[1]) {
        return;
    }
    cleanup();

    tft.setOrigin(center_x, center_y);
    const uint32_t t_height = radius - 2;
    float angle_x = radians(vector[0]);
    tft.fillTriangle(t_height*sin(angle_x), t_height*-cos(angle_x), base_length/2*cos(angle_x), base_length/2*sin(angle_x), 
            base_length/2*-cos(angle_x), base_length/2*-sin(angle_x), TFT_RED);
    tft.fillTriangle(t_height*-sin(angle_x), t_height*cos(angle_x), base_length/2*cos(angle_x), base_length/2*sin(angle_x), 
            base_length/2*-cos(angle_x), base_length/2*-sin(angle_x), TFT_SKYBLUE);
    tft.setOrigin(0, 0);
    vector_old[0] = vector[0];
    vector_old[1] = vector[1];
    vector_old[2] = vector[2];
}


void showMotorData(const MotorData &motorData, const uint8_t num_of_motor, const uint32_t background_color) { //現状LEGACYにしか対応してない
    switch (num_of_motor) {
        case 2:
            tft.setTextSize(2);
            tft.fillRect(0, tft.height() - tft.fontHeight(), tft.width(), tft.fontHeight(), background_color);
            tft.setCursor(tft.width() - tft.textWidth("L:     R:    "), tft.height() - tft.fontHeight());
            tft.print("L:");
            tft.print(motorData.power[0]);
            tft.setCursor(tft.width() - tft.textWidth("R:    "), tft.height() - tft.fontHeight());
            tft.print("R:");
            tft.print(motorData.power[1]);
            break;
    }
}

#endif