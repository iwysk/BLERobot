#include <Arduino.h>
#include "base_service.hpp"

// uuid generator
// https://www.uuidgenerator.net/ 
BLEUUID name_char_uuid = BLEUUID((uint16_t)0x2A00);
BLEUUID battery_level_char_uuid = BLEUUID((uint16_t)0x2A19);
BLEUUID command_char_uuid = BLEUUID("ed25d4db-ebb5-4072-bacc-759166b134a4");
constexpr Command command_null = {.command = 0, .parameter = 0};

BaseService::BaseService(BLEUUID _service_uuid) : bInitialized(false), bActivated(false), pService(nullptr), pNameChar(nullptr), pBatteryLevelChar(nullptr), 
 pCommandChar(nullptr), service_uuid(_service_uuid) {
    ESP_LOGV(TAG, "constructor");
}

void BaseService::initService(BLEServer* pServer) {
    ESP_LOGV(TAG, ">> initService");
    pService = pServer->createService(service_uuid);
    pNameChar = pService->createCharacteristic(name_char_uuid, BLECharacteristic::PROPERTY_WRITE);
    pNameChar->setValue(std::string(""));
    pNameChar->setCallbacks(new NameCharCallbacks(this));

    pBatteryLevelChar = pService->createCharacteristic(battery_level_char_uuid, BLECharacteristic::PROPERTY_WRITE);
    uint8_t battery_level_initial_value = 0;
    pBatteryLevelChar->setValue(&battery_level_initial_value, 1);

    pCommandChar = pService->createCharacteristic(command_char_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    pCommandChar->addDescriptor(new BLE2902());
    uint8_t data[sizeof(Command)];
    memcpy(data, &command_null, sizeof(command_null));
    pCommandChar->setValue(data, sizeof(command_null));
    ESP_LOGV(TAG, "initService <<");
}

bool BaseService::getInitialized(void) const {
    return bInitialized;
}


void BaseService::startService(void) {
    ESP_LOGV(TAG, ">> startService");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        return;
    }
    pService->start();
    ESP_LOGV(TAG, "startService <<");
}


std::string BaseService::getName(void) {
    ESP_LOGV(TAG, ">> getName");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        return std::string("");
    }
    return pNameChar->getValue();
    ESP_LOGV(TAG, "getName <<");
}

void BaseService::getBatteryLevel(uint8_t& battery_level) {
    ESP_LOGV(TAG, ">> getBatteryLevel");
    getData(pBatteryLevelChar, battery_level);
    ESP_LOGV(TAG, "getBatteryLevel <<");
}


void BaseService::setBatteryLevelCharCallback(BatteryLevelCharCallbackFunc_t _batteryLevelCbFunc) {
    ESP_LOGV(TAG, ">> setBatteryLevelCharCallback");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        return;
    }
    if (nullptr == _batteryLevelCbFunc) {
        ESP_LOGE(TAG, "batteryLevelCbFunc is nullptr.");
        return;
    }
    pBatteryLevelChar->setCallbacks(new BatteryLevelCharCallbacks(_batteryLevelCbFunc, TAG));
    ESP_LOGV(TAG, "setBatteryLevelCharCallback <<");
}

void BaseService::setCommand(const Command& command) {
    ESP_LOGV(TAG, ">> setCommand");
    uint8_t data[sizeof(Command)];
    memcpy(data, &command, sizeof(Command));
    setData(pCommandChar, data, sizeof(Command));
    pCommandChar->notify();
    ESP_LOGV(TAG, "setCommand <<");   
}

void BaseService::getCommand(Command& command) {
    ESP_LOGV(TAG, ">> getCommand");
    uint8_t data[sizeof(Command)];
    getData(pCommandChar, data, sizeof(Command));
    memcpy(&command, data, sizeof(Command));
    ESP_LOGV(TAG, "getCommand <<");
}

bool BaseService::getData(BLECharacteristic* &pChar, uint8_t* data, const size_t &size) {
    ESP_LOGV(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        return false;
    }
    std::string data_str = pChar->getValue();
    if (data_str.size() != size) {
        ESP_LOGE(TAG, "Eroor: characteristic data size doesn't match.");
        ESP_LOGE(TAG, "expected data size:%d, char data size:%d", size, data_str.size());
        return false;
    }
    memcpy(data, data_str.c_str(), data_str.size());
    ESP_LOGV(TAG, "getData <<");
    return true;
}

void BaseService::setData(BLECharacteristic* &pChar, uint8_t* data, const size_t &size) {
    ESP_LOGV(TAG, ">> setData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        return;
    }
    pChar->setValue(data, size);
    ESP_LOGV(TAG, "setData <<");
}

void BaseService::getData(BLECharacteristic* &pChar, uint8_t &data) {
    ESP_LOGV(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        return;
    }
    std::string data_str = pChar->getValue();
    data = data_str[0];
    ESP_LOGV(TAG, "getData <<");
}

void BaseService::setData(BLECharacteristic* &pChar, uint8_t &data) {
    ESP_LOGV(TAG, ">> setData");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        ESP_LOGV(TAG, "setData <<");
        return;
    }
    pChar->setValue(&data, 1);
    ESP_LOGV(TAG, "setData <<");
}

bool BaseService::isActivated(void) const {
    ESP_LOGV(TAG, ">> isActivated <<");
    return bActivated;
}

void BaseService::cleanUp(void) {
    ESP_LOGV(TAG, ">> cleanUp");
    if (!bInitialized) {
        ESP_LOGE(TAG, "BaseService is not Initialized.");
        ESP_LOGV(TAG, "cleanUp <<");
        return;
    }
    pNameChar->setValue(std::string(""));
    uint8_t battery_level_initial_value = 0;
    pBatteryLevelChar->setValue(&battery_level_initial_value, 1);
    uint8_t data[sizeof(Command)];
    memcpy(data, &command_null, sizeof(command_null));
    pCommandChar->setValue(data, sizeof(Command));
    ESP_LOGV(TAG, "cleanUp <<");
}

NameCharCallbacks::NameCharCallbacks(BaseService* _service) : service(_service), TAG("BLECallback") {
    ESP_LOGV(TAG, "constructor");
}

void NameCharCallbacks::onWrite(BLECharacteristic* pNameChar) {
    std::string Name = pNameChar->getValue();
    if (Name == "") {
        service->bActivated = false;
        ESP_LOGI(service->TAG, "is deactivated");
    }
    else {
        service->bActivated = true;
        ESP_LOGI(service->TAG, "is activated");
        ESP_LOGI(service->TAG, "Name: %s", Name.c_str());
    }
}

BatteryLevelCharCallbacks::BatteryLevelCharCallbacks(BatteryLevelCharCallbackFunc_t _batteryLevelCbFunc, const char* _type) : 
batteryLevelCbFunc(_batteryLevelCbFunc), TAG("BLECallback"), type(_type) {
    ESP_LOGV(TAG, "constructor");
}

void BatteryLevelCharCallbacks::onWrite(BLECharacteristic* pBatteryLevelChar) {
    ESP_LOGV(TAG, ">> onWrite");
    const uint8_t battery_level = pBatteryLevelChar->getValue().c_str()[0];
    if (nullptr == batteryLevelCbFunc) {
        ESP_LOGW(TAG, "Pointer to callback func is nullptr.");
        return;
    }
    batteryLevelCbFunc(type, battery_level);
    ESP_LOGV(TAG, "onWrite <<");
}