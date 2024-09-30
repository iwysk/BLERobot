#include <Arduino.h>
#include "service_manager.hpp"

const BLEUUID name_char_uuid = BLEUUID((uint16_t)0x2A00);
const BLEUUID battery_level_char_uuid = BLEUUID((uint16_t)0x2A19);
const BLEUUID command_char_uuid = BLEUUID("ed25d4db-ebb5-4072-bacc-759166b134a4");

ServiceManager::ServiceManager(BLEUUID _service_uuid)
 : TAG("Manager"), bInitialized(false), pClient(nullptr), pService(nullptr), pBatteryLevelChar(nullptr), 
   service_uuid(_service_uuid)
{
    esp_log_level_set(TAG, ESP_LOG_WARN);
    ESP_LOGV(TAG, "constructor");
}

ServiceManager::~ServiceManager(void) 
{
    ESP_LOGV(TAG, "destructor");
}


//サービスが見つからなかった場合bInitializedをfalseにする　このクラス内にbInitializeをtrueにする文は一つもない
//オーバーライドしているクラスのinit()で、すべての特性が見つかった場合のみtrueになる

bool ServiceManager::InitService(void) 
{
    ESP_LOGV(TAG, ">> InitService");
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Client is not connected.");
        ESP_LOGV(TAG, "InitService <<");
        return false;
    }
    pService = pClient->getService(service_uuid);
    if (nullptr == pService) {
        bInitialized = false;
        ESP_LOGE(TAG, "Service was not found.");
        ESP_LOGV(TAG, "InitService <<");
        return false;
    }
    ESP_LOGI(TAG, "Found service."); 
    pNameChar = pService->getCharacteristic(name_char_uuid);
    pBatteryLevelChar = pService->getCharacteristic(battery_level_char_uuid);
    pCommandChar = pService->getCharacteristic(command_char_uuid);
    ESP_LOGV(TAG, "InitService <<");
    return true;
}

bool ServiceManager::getInitialized(void) const {
    return bInitialized;
}


bool ServiceManager::getData(BLERemoteCharacteristic* &pChar, uint8_t* data, const size_t &size) {
    ESP_LOGV(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Unit manager is not initialized.");
        ESP_LOGV(TAG, "getData <<");
        return false;
    }
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Eroor: Client is not connected.");
        ESP_LOGV(TAG, "getData <<");
    }
    if (!pChar->canRead()) {
        ESP_LOGE(TAG, "Eroor: Can't read characteristic.");
        ESP_LOGV(TAG, "getData <<");
        return false;
    }
    ESP_LOGV(TAG, "Successed to read characteristic.");
    std::string data_str = pChar->readValue();
    if (data_str.size() != size) {
        ESP_LOGE(TAG, "Eroor: characteristic data size doesn't match.");
        ESP_LOGE(TAG, "expected data size:%d, char data size:%d", size, data_str.size());
        ESP_LOGV(TAG, "getData <<");
        return false;
    }
    memcpy(data, data_str.c_str(), data_str.size());
    ESP_LOGV(TAG, "getData <<");
    return true;
}



bool ServiceManager::setData(BLERemoteCharacteristic* &pChar, uint8_t* data, const size_t &size) {
    ESP_LOGV(TAG, ">> setData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: unit manager is not initialized.");
        ESP_LOGV(TAG, "setData <<");
        return false;
    }
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Eroor: Client is not connected.");
        ESP_LOGV(TAG, "setData <<");
    }
    if (!pChar->canWrite()) {
        ESP_LOGE(TAG, "Eroor: Can't write value to characteristic.");
        ESP_LOGV(TAG, "setData <<");
        return false;
    }
    uint16_t mtu = pClient->getMTU();
    if (size <= mtu - 3) {
        pChar->writeValue(data, size, false);
    }
    else {
        ESP_LOGW(TAG, "Eroor: Max data size is %d.", mtu - 3);
    }
    ESP_LOGV(TAG, "Successed to write value to characteristic.");
    ESP_LOGV(TAG, "setData <<");
    return true;
}


bool ServiceManager::getData(BLERemoteCharacteristic* &pChar, uint8_t& data) {
    ESP_LOGV(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Unit manager is not initialized.");
        ESP_LOGV(TAG, "getData <<");
        return false;
    } 
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Eroor: Client is not connected.");
        ESP_LOGV(TAG, "getData <<");
        return false;
    }
    if (!pChar->canRead()) {
        ESP_LOGE(TAG, "Eroor: Can't read characteristic.");
        ESP_LOGV(TAG, "getData <<");
        return false;
    }
    data = pChar->readUInt8();
    ESP_LOGD(TAG, "Successed to read characteristic.");
    ESP_LOGV(TAG, "getData <<");
    return true;
}


bool ServiceManager::setData(BLERemoteCharacteristic* &pChar, const uint8_t& data) {
    ESP_LOGV(TAG, ">> setData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: unit manager is not initialized.");
        ESP_LOGV(TAG, "setData <<");
        return false;
    }
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Eroor: Client is not connected.");
        ESP_LOGV(TAG, "setData <<");
        return false;
    }
    if (!pChar->canWrite()) {
        ESP_LOGE(TAG, "Eroor: Can't write value to characteristic.");
        ESP_LOGV(TAG, "setData <<");
        return false;
    }
    pChar->writeValue(data);
    ESP_LOGD(TAG, "Successed to write value to characteristic.");
    ESP_LOGV(TAG, "setData <<");
    return true;
}


bool ServiceManager::setName(std::string Name) {
    ESP_LOGV(TAG, ">> setName");
    bool result = setData(pNameChar, (uint8_t*)Name.c_str(), Name.size());
    ESP_LOGV(TAG, "setName <<");
    return result;
} 

bool ServiceManager::setBatteryLevel(const uint8_t &battery_level) {
    ESP_LOGV(TAG, ">> getBatteryLevel");
    bool result = setData(pBatteryLevelChar, battery_level);
    ESP_LOGV(TAG, "getBatteryLevel <<");
    return result;
}


bool ServiceManager::setCommand(const Command& command) {
    ESP_LOGV(TAG, ">> setCommand");
    uint8_t data[sizeof(Command)];
    memcpy(data, &command, sizeof(Command));
    bool result = setData(pCommandChar, data, sizeof(Command));
    ESP_LOGV(TAG, "setCommand <<");
    return result;
}

bool ServiceManager::getCommand(Command& command) {
    ESP_LOGV(TAG, ">> getCommand");
    uint8_t data[sizeof(Command)];
    if (!getData(pCommandChar, data, sizeof(Command))) {
        return false;
    }
    memcpy(&command, data, sizeof(Command));
    ESP_LOGV(TAG, "getCommand <<");
    return true;
}

bool ServiceManager::setCommandCallback(CommandCharCallbackFunc_t _CommandCbFunc) {
    ESP_LOGV(TAG, ">> setCommandCallback");  
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: unit manager is not initialized.");
        ESP_LOGV(TAG, "setCommandCallback <<");
        return false;
    }
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Eroor: Client is not connected.");
        ESP_LOGV(TAG, "setCommandCallback <<");
        return false;
    }
    pCommandChar->registerForNotify(_CommandCbFunc);
    ESP_LOGV(TAG, "setCommandCallback <<"); 
    return true;  
}