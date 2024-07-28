#include <Arduino.h>
#include "unit_manager.hpp"


const BLEUUID UnitManager::battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

UnitManager::UnitManager(BLEUUID _service_uuid)
 : bInitialized(false), TAG("Unit"), pService(nullptr), pBatteryLevelChar(nullptr), 
   service_uuid(_service_uuid)
{
    ESP_LOGD(TAG, "UnitManager()");
}

UnitManager::~UnitManager(void) 
{

}


bool UnitManager::InitService(BLEClient *pClient) 
{
    pService = pClient->getService(service_uuid);
    if (nullptr == pService) {
        bInitialized = false;
        ESP_LOGW(TAG, "Service was not found.");
        return false;
    }
    ESP_LOGI(TAG, "Found service."); 
    pBatteryLevelChar = pService->getCharacteristic(battery_level_char_uuid);
    return true;
}

bool UnitManager::getInitialized(void) const {
    return bInitialized;
}


bool UnitManager::getData(BLERemoteCharacteristic* &pChar, uint8_t* data, const size_t &size) {
    ESP_LOGD(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Unit manager is not initialized.");
        return false;
    }
    if (!pChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read characteristic.");
        return false;
    }
    ESP_LOGD(TAG, "Successed to read characteristic.");
    std::string data_str = pChar->readValue();
    if (data_str.size() != size) {
        ESP_LOGW(TAG, "Eroor: characteristic data size doesn't match.");
        ESP_LOGW(TAG, "expected data size:%d, char data size:%d", size, data_str.size());
        return false;
    }
    memcpy(data, data_str.c_str(), data_str.size());
    ESP_LOGD(TAG, "getData <<");
    return true;
}



bool UnitManager::setData(BLERemoteCharacteristic* &pChar, uint8_t* data, const size_t &size) {
    ESP_LOGD(TAG, ">> setData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: unit manager is not initialized.");
        return false;
    }
    if (!pChar->canWrite()) {
        ESP_LOGW(TAG, "Eroor: Can't write value to characteristic.");
        return false;
    }
    pChar->writeValue(data, size, false);
    ESP_LOGD(TAG, "Successed to write value to characteristic.");
    ESP_LOGD(TAG, "setData <<");
    return true;
}


bool UnitManager::getData(BLERemoteCharacteristic* &pChar, uint8_t& data) {
    ESP_LOGD(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Unit manager is not initialized.");
        return false;
    } 
    if (!pChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read characteristic.");
        return false;
    }
    data = pChar->readUInt8();
    ESP_LOGD(TAG, "Successed to read characteristic.");
    ESP_LOGD(TAG, "getData <<");
    return true;
}


bool UnitManager::setData(BLERemoteCharacteristic* &pChar, const uint8_t& data) {
    ESP_LOGD(TAG, ">> setData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: unit manager is not initialized.");
        return false;
    }
    if (!pChar->canWrite()) {
        ESP_LOGW(TAG, "Eroor: Can't write value to characteristic.");
        return false;
    }
    pChar->writeValue(data);
    ESP_LOGD(TAG, "Successed to write value to characteristic.");
    ESP_LOGD(TAG, "setData <<");
    return true;
}


bool UnitManager::getBatteryLevel(uint8_t &battery_level) {
    ESP_LOGD(TAG, ">> getBatteryLevel");
    if (!getData(pBatteryLevelChar, battery_level)) {
        ESP_LOGD(TAG, "getBatteryLevel <<");
        return false;
    }
    ESP_LOGI(TAG, "Battery level: %d", battery_level);
    ESP_LOGD(TAG, "getBatteryLevel <<");
    return true;
}


bool UnitManager::sendCommand(BLERemoteCharacteristic* &pCommandChar, const Command& command) {
    ESP_LOGD(TAG, ">> setCommand");
    uint8_t data[50];
    memcpy(data, &command, sizeof(command));
    bool result = setData(pCommandChar, data, sizeof(command));
    ESP_LOGD(TAG, "setCommand <<");
    return result;
}