#include "linetracer_service.hpp"

BLEUUID linedata_char_uuid = BLEUUID("4aca4700-c7de-446b-9961-f8731697b5ed");
BLEUUID linetracer_service_uuid = BLEUUID("d2c7abdd-7dd5-4b75-83c5-d3d972ce2963");

LineTracerService* LineTracerService::lineTracer = nullptr;

LineTracerService::LineTracerService(void) : BaseService(linetracer_service_uuid) {
    strcpy(TAG, "LineTracer");
    ESP_LOGD(TAG, "constructor");
}

LineTracerService::~LineTracerService(void) {
    ESP_LOGD(TAG, "destructor");
}

LineTracerService* LineTracerService::getInstance(void) {
    ESP_LOGD("LineTracerService", ">> getInstance");
    if (nullptr == lineTracer) {
        lineTracer = new LineTracerService();
    }
    ESP_LOGD("LineTracerService", "getInstance <<");
    return lineTracer;
}

void LineTracerService::init(BLEServer* pServer) {
    ESP_LOGD(TAG, ">> init");
    initService(pServer);
    pLineDataChar = pService->createCharacteristic(linedata_char_uuid, BLECharacteristic::PROPERTY_WRITE);
    bInitialized = true;
    ESP_LOGD(TAG, "init <<");
}


bool LineTracerService::getLineData(LineData& lineData) {
    ESP_LOGD(TAG, ">> getLineData"); 
    uint8_t data[sizeof(LineData)];
    if (getData(pLineDataChar, data, sizeof(LineData))) {
        ESP_LOGD(TAG, "getLineData <<");   
        return false;
    }
    memcpy(&lineData, data, sizeof(LineData));
    ESP_LOGD(TAG, "getLineData <<");   
    return true;
}