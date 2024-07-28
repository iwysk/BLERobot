#include <Arduino.h>
#include "arm_service_manager.hpp"

const BLEUUID arm_service_uuid = BLEUUID("f5c73196-04bc-497b-b6c4-dc10a98b4d41");
const BLEUUID arm_char_uuid = BLEUUID("841e98ea-cf36-43c5-be86-18c52434757c");
const BLEUUID ball_count_char_uuid = BLEUUID("69338920-a602-424b-8992-400a0fa0187c");

ArmServiceManager* ArmServiceManager::arm = nullptr;

ArmServiceManager::ArmServiceManager(void) : ServiceManager(arm_service_uuid), pArmChar(nullptr), pBallCountChar(nullptr)  {
    strcpy(TAG, "ArmUnit");
    esp_log_level_set(TAG, ESP_LOG_WARN);
    ESP_LOGV(TAG, "constructor");
}

ArmServiceManager::~ArmServiceManager(void) {
    ESP_LOGV(TAG, "destructor");
}

ArmServiceManager* ArmServiceManager::getInstance(void) {
    ESP_LOGV("ArmServiceManager", ">> getInstance");
    if (nullptr == arm) {
        arm = new ArmServiceManager();
    }
    ESP_LOGV("ArmServiceManager", "getInstance <<");
    return arm;
}


bool ArmServiceManager::init(BLEClient *_pClient) {
    ESP_LOGV(TAG, ">> init");
    pClient = _pClient;
    if (InitService()) {
        pArmChar = pService->getCharacteristic(arm_char_uuid);
        pBallCountChar = pService->getCharacteristic(ball_count_char_uuid);
        if (nullptr != pArmChar && nullptr != pBallCountChar) {
            bInitialized = true;
        }
    }
    ESP_LOGV(TAG, "init <<");
    return bInitialized;
}


bool ArmServiceManager::getArmData(ArmData& armData) {
    ESP_LOGV(TAG, ">> getArmData");
    uint8_t data[sizeof(ArmData)];
    bool result = getData(pArmChar, data, sizeof(ArmData));
    if (result) {
        memcpy(&armData, data, sizeof(ArmData));
        ESP_LOGI(TAG, "armData: motorPower[0]:%d motorPower[1]:%d sensor_value[0]:%f, sensor_value[1]:%f, servo_angle[0]:%d, servo_angle[1]:%d", 
                    armData.motorPower[0], armData.motorPower[1], armData.sensor_value[0], armData.sensor_value[1], armData.servo_angle[0], armData.servo_angle[1]);
    }
    ESP_LOGV(TAG, "getArmData <<");
    return result;
}


bool ArmServiceManager::setArmData(const ArmData& armData) {
    ESP_LOGV(TAG, ">> setArmData");
    uint8_t data[sizeof(ArmData)];
    memcpy(data, &armData, sizeof(ArmData));
    bool result = setData(pArmChar, data, sizeof(ArmData));
    ESP_LOGV(TAG, "setArmData <<");
    return result;
}


bool ArmServiceManager::getBallCount(uint8_t& ball_count) {
    ESP_LOGV(TAG, ">> getBallCount");
    bool result = getData(pBallCountChar, ball_count);
    if (result) {
        ESP_LOGD(TAG, "ball_count: %d", ball_count);
    }
    else {
        ESP_LOGW(TAG, "Failed to read ball count.");
    }
    ESP_LOGV(TAG, "getBallCount <<");
    return result;
}


