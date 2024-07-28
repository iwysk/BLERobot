#include <Arduino.h>
#include <BLEDevice.h>
#include "linetracer_unit_manager.hpp"

LineTracerUnitManager* LineTracerUnitManager::lineTracerUnit = nullptr;
const BLEUUID LineTracerUnitManager::linetracer_service_uuid = BLEUUID("d2c7abdd-7dd5-4b75-83c5-d3d972ce2963");
const BLEUUID LineTracerUnitManager::line_data_char_uuid = BLEUUID("4aca4700-c7de-446b-9961-f8731697b5ed");

LineTracerUnitManager::LineTracerUnitManager(void) : UnitManager(linetracer_service_uuid), pLineDataChar(nullptr) {
    strcpy(TAG, "lineTracerUnit");
    ESP_LOGD(TAG, "LineTracerUnitManager()\n");
}


LineTracerUnitManager::~LineTracerUnitManager(void) {
    ESP_LOGD(TAG, "~LineTracerUnitManager()\n");
}


LineTracerUnitManager* LineTracerUnitManager::getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized) {
    ESP_LOGD(TAG, ">> getInstance");
    if (nullptr == lineTracerUnit) {
        lineTracerUnit = new LineTracerUnitManager();
    }
    if (bInitialize) {
        lineTracerUnit->init(pClient);
    }
    bInitialized = lineTracerUnit->bInitialized;
    ESP_LOGD(TAG, "getInstance <<\n");
    return lineTracerUnit;
}


bool LineTracerUnitManager::init(BLEClient* pClient) {
    ESP_LOGD(TAG, ">> init");
    if (InitService(pClient)) {;
        pLineDataChar = pService->getCharacteristic(line_data_char_uuid);
        if (nullptr != pLineDataChar) {
            bInitialized = true;
        }
    }
    ESP_LOGD(TAG, ">> init");
    return bInitialized;
}


bool LineTracerUnitManager::getLineData(LineData& lineData) {
    ESP_LOGD(TAG, ">> getLineData");
    uint8_t data[50];
    bool result = getData(pLineDataChar, data, sizeof(lineData));
    if (result) {
        memcpy(&lineData, data, sizeof(lineData));
        ESP_LOGD(TAG, "lineData, num_of_reflector:%d, bValue:%d, isInterSection:%d", lineData.num_of_reflector, lineData.bValue, lineData.isInterSection);
    }
    else {
        ESP_LOGW(TAG, "Failed to read Line Data.");
    }
    ESP_LOGD(TAG, "getLineData <<");
    return result;
}
