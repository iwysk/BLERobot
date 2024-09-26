#include "arm_service.hpp"

BLEUUID arm_service_uuid = BLEUUID("f5c73196-04bc-497b-b6c4-dc10a98b4d41");
BLEUUID arm_char_uuid = BLEUUID("841e98ea-cf36-43c5-be86-18c52434757c");
BLEUUID ball_count_char_uuid = BLEUUID("69338920-a602-424b-8992-400a0fa0187c");

ArmService* ArmService::arm = nullptr;
ArmData armData_null = {.motorPower = {0, 0}, .servo_angle = {0, 0}, .sensor_value = {0, 0}};

ArmService::ArmService(void) : BaseService(arm_service_uuid), pArmChar(nullptr), pBallCountChar(nullptr) {
    strcpy(TAG, "Arm");
    ESP_LOGV(TAG, "constructor");
}

ArmService::~ArmService(void) {
    ESP_LOGV(TAG, "destructor");
}

ArmService* ArmService::getInstance(void) {
    ESP_LOGV("Arm", ">> getInstance");
    if (nullptr == arm) {
        arm = new ArmService();
    }
    ESP_LOGV("Arm", "getInstance <<");
    return arm;
}

void ArmService::init(BLEServer *pServer) { //bInitializedはここでのみtrueになる
    ESP_LOGV(TAG, ">> init");
    initService(pServer);
    pArmChar = pService->createCharacteristic(arm_char_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    uint8_t data[sizeof(ArmData)];
    memcpy(data, &armData_null, sizeof(ArmData));
    pArmChar->setValue(data, sizeof(ArmData));
    pBallCountChar = pService->createCharacteristic(ball_count_char_uuid, BLECharacteristic::PROPERTY_WRITE);
    uint8_t ball_count_initial = 0;
    pBallCountChar->setValue(&ball_count_initial, 1);
    bInitialized = true;
    ESP_LOGV(TAG, "init <<");
}


bool ArmService::getArmData(ArmData& armData) {
    ESP_LOGV(TAG, ">> getArmData");
    uint8_t data[sizeof(ArmData)];
    if (!getData(pArmChar, data, sizeof(ArmData))) {
        ESP_LOGV(TAG, "getArmData <<");
        return false;
    }
    memcpy(&armData, data, sizeof(ArmData));
    ESP_LOGV(TAG, "getArmData <<");
    return true;
}

void ArmService::setArmdata(const ArmData& armData) {
    ESP_LOGV(TAG, ">> setArmData");
    uint8_t data[sizeof(ArmData)];
    memcpy(data, &armData, sizeof(ArmData));
    setData(pArmChar, data, sizeof(ArmData));
    ESP_LOGV(TAG, "setArmData <<");
}

void ArmService::getBallCount(uint8_t& ball_count) {
    ESP_LOGV(TAG, ">> getBallCount");
    getData(pBallCountChar, ball_count);
    ESP_LOGV(TAG, "getBallCount <<");
}

void ArmService::setBallCountCharCallback(BallCountCharCallbackFunc_t _ballCountCbFunc) {
    ESP_LOGV(TAG, ">> setBallCountCharCallback");
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Not initialized.");
        return;
    }
    pBallCountChar->setCallbacks(new BallCountCharCallbacks(_ballCountCbFunc));
    ESP_LOGV(TAG, "setBallCountCharCallback <<");
}


BallCountCharCallbacks::BallCountCharCallbacks(BallCountCharCallbackFunc_t _ballCountCbFunc) : ballCountCbFunc(_ballCountCbFunc), TAG("BLECallback"){
    ESP_LOGV(TAG, "constructor");
}

void BallCountCharCallbacks::onWrite(BLECharacteristic* pBallCountChar) {
    ESP_LOGV(TAG, ">> onWrite");
    uint8_t ball_count = pBallCountChar->getValue().c_str()[0];
    if (nullptr == ballCountCbFunc) {
        ESP_LOGW(TAG, "Pointer to callback func is nullptr.");
        return;
    }
    ballCountCbFunc(ball_count);
    ESP_LOGV(TAG, "onWrite <<");
}