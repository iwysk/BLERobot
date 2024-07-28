#include "linetracer_service_manager.hpp"

const BLEUUID linetracer_service_uuid = BLEUUID("d2c7abdd-7dd5-4b75-83c5-d3d972ce2963");
const BLEUUID line_data_char_uuid = BLEUUID("4aca4700-c7de-446b-9961-f8731697b5ed");

LineTracerServiceManager* LineTracerServiceManager::lineTracer = nullptr;

LineTracerServiceManager::LineTracerServiceManager(void) : ServiceManager(linetracer_service_uuid), pLineDataChar(nullptr) {
    strcpy(TAG, "lineTracer");
    esp_log_level_set(TAG, ESP_LOG_WARN);
    ESP_LOGV(TAG, "LineTracerServiceManager()");
}


LineTracerServiceManager::~LineTracerServiceManager(void) {
    ESP_LOGV(TAG, "~LineTracerServiceManager()");
}


LineTracerServiceManager* LineTracerServiceManager::getInstance(void) {
    ESP_LOGV("LineTracerServiceManager", ">> getInstance");
    if (nullptr == lineTracer) {
        lineTracer = new LineTracerServiceManager();
    }
    ESP_LOGV("LineTracerServiceManager", "getInstance <<");
    return lineTracer;
}


bool LineTracerServiceManager::init(BLEClient* _pClient) {
    ESP_LOGV(TAG, ">> init");
    pClient = _pClient;
    if (InitService()) {
        pLineDataChar = pService->getCharacteristic(line_data_char_uuid);
        if (nullptr != pLineDataChar) {
            bInitialized = true;
        }
    }
    ESP_LOGV(TAG, ">> init");
    return bInitialized;
}


bool LineTracerServiceManager::getLineData(LineData& lineData) {
    ESP_LOGV(TAG, ">> getLineData");
    uint8_t data[sizeof(LineData)];
    bool result = getData(pLineDataChar, data, sizeof(LineData));
    if (result) {
        memcpy(&lineData, data, sizeof(LineData));
        ESP_LOGD(TAG, "lineData, num_of_reflector:%d, bValue:%d, isInterSection:%d", lineData.num_of_reflector, lineData.bValue, lineData.isInterSection);
    }
    else {
        ESP_LOGW(TAG, "Failed to read Line Data.");
    }
    ESP_LOGV(TAG, "getLineData <<");
    return result;
}
