#include <Arduino.h>
#include <BLEDevice.h>
#include "arm_unit_manager.hpp"


ArmUnitManager* ArmUnitManager::armUnit = nullptr;
const BLEUUID ArmUnitManager::arm_service_uuid = BLEUUID("f5c73196-04bc-497b-b6c4-dc10a98b4d41");
const BLEUUID ArmUnitManager::arm_char_uuid = BLEUUID("841e98ea-cf36-43c5-be86-18c52434757c");
const BLEUUID ArmUnitManager::ball_count_char_uuid = BLEUUID("69338920-a602-424b-8992-400a0fa0187c");


ArmUnitManager::ArmUnitManager(void) : UnitManager(arm_service_uuid), pArmChar(nullptr), pBallCountChar(nullptr)  {
    strcpy(TAG, "ArmUnit");
    ESP_LOGD(TAG, "ArmUnitManager()");
}

ArmUnitManager::~ArmUnitManager(void) {
    ESP_LOGD(TAG, "~ArmUnitManager()");
}

ArmUnitManager* ArmUnitManager::getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized) {
    ESP_LOGD(TAG, ">> getInstance");
    if (nullptr == armUnit) {
        armUnit = new ArmUnitManager();
    }
    if (bInitialize) {
        armUnit->init(pClient);
    }
    bInitialized = armUnit->bInitialized;
    ESP_LOGD(TAG, "getInstance <<\n");
    return armUnit;
}


bool ArmUnitManager::init(BLEClient *pClient) {
    ESP_LOGD(TAG, ">> init");
    if (InitService(pClient)) {
        pArmChar = pService->getCharacteristic(arm_char_uuid);
        pBallCountChar = pService->getCharacteristic(ball_count_char_uuid);
        if (nullptr != pArmChar && nullptr != pBallCountChar) {
            bInitialized = true;
        }
    }
    ESP_LOGD(TAG, "init <<");
    return bInitialized;
}


bool ArmUnitManager::getArmData(ArmData& armData) {
    ESP_LOGD(TAG, ">> getArmData");
    uint8_t data[50];
    bool result = getData(pArmChar, data, sizeof(armData));
    if (result) {
        memcpy(&armData, data, sizeof(armData));
        ESP_LOGI(TAG, "armData: motorPower[0]:%d motorPower[1]:%d sensor_value[0]:%f, sensor_value[1]:%f, servo_angle[0]:%d, servo_angle[1]:%d", 
                    armData.motorPower[0], armData.motorPower[1], armData.sensor_value[0], armData.sensor_value[1], armData.servo_angle[0], armData.servo_angle[1]);
    }
    ESP_LOGD(TAG, "getArmData <<");
    return result;
}


bool ArmUnitManager::setArmData(const ArmData& armData) {
    ESP_LOGD(TAG, ">> setArmData");
    uint8_t data[50];
    memcpy(data, &armData, sizeof(armData));
    bool result = setData(pArmChar, data, sizeof(armData));
    ESP_LOGD(TAG, "setArmData <<");
    return result;
}


bool ArmUnitManager::getBallCount(uint8_t& ball_count) {
    ESP_LOGD(TAG, ">> getBallCount");
    bool result = getData(pBallCountChar, ball_count);
    if (result) {
        ESP_LOGI(TAG, "ball_count: %d", ball_count);
    }
    else {
        ESP_LOGW(TAG, "Failed to read ball count.");
    }
    ESP_LOGD(TAG, "getBallCount <<");
    return result;
}


