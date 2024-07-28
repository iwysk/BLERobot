#include <Arduino.h>
#include "main_service.hpp"

//getDataがバグらないかテストしたい
BLEUUID main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
BLEUUID motor_char_uuid = BLEUUID("f4c7506e-557c-4510-bc11-087c1a776155");
BLEUUID bno_char_uuid = BLEUUID("43225596-4ab8-4493-b4f3-e065a5eeb636");

bool MotorData::operator==(const MotorData& motorData) const {
    bool result;
    result = (num_of_motor == motorData.num_of_motor);
    result = (result && (power[0] == motorData.power[0]));
    result = (result && (power[1] == motorData.power[1]));
    result = (result && (power[2] == motorData.power[2]));
    result = (result && (power[3] == motorData.power[3]));
    return result;
}

bool MotorData::operator!=(const MotorData& motorData) const {
    return !(*this == motorData);
}

MainService* MainService::mainService = nullptr;

MainService::MainService(void) : BaseService(main_service_uuid), pMotorChar(nullptr), pBNOChar(nullptr) {
    strcpy(TAG, "Main");
    esp_log_level_set(TAG, ESP_LOG_NONE);
    ESP_LOGV(TAG, "constructor");
}

MainService::~MainService(void) {
    ESP_LOGV(TAG, "destructor");
}


MainService* MainService::getInstance(void) {
    ESP_LOGV("Main", ">> getInstance");
    if (nullptr == mainService) {
        mainService = new MainService();
    }
    ESP_LOGV("Main", "getInstance <<");
    return mainService;
}

void MainService::init(BLEServer* pServer) {  //bInitializedはここでしかtrueにならない
    ESP_LOGV(TAG, ">> init");
    initService(pServer);
    pMotorChar = pService->createCharacteristic(motor_char_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    pMotorChar->addDescriptor(new BLE2902());
    pBNOChar = pService->createCharacteristic(bno_char_uuid, BLECharacteristic::PROPERTY_WRITE);
    bInitialized = true;
    ESP_LOGV(TAG, "init <<");
}

bool MainService::setMotorData(const MotorData& motorData) {
    ESP_LOGV(TAG, ">> setMotorData");
    uint8_t data[sizeof(MotorData)];
    memcpy(data, &motorData, sizeof(MotorData));
    setData(pMotorChar, data, sizeof(MotorData));
    pMotorChar->notify();
    ESP_LOGV(TAG, "setMotorData <<");
    return true;
}

void MainService::getBnoData(BnoData& bnoData) {
    ESP_LOGV(TAG, ">> getBNOData");
    uint8_t data[sizeof(BnoData)];
    getData(pBNOChar, data, sizeof(BnoData));
    memcpy(&bnoData, data, sizeof(BnoData));
    ESP_LOGV(TAG, "getBNOData <<"); 
}

void MainService::setBnoCharCallback(BnoCharCallbackFunc_t _bnoCbFunc) {
    ESP_LOGV(TAG, ">> setBnoCharCallback"); 
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Not initialized.");
        return;
    }
    if (_bnoCbFunc == nullptr) {
        ESP_LOGE(TAG, "Eroor: _bnoCbFunc is nullptr.");
        ESP_LOGV(TAG, "setBnoCharCallback <<"); 
        return;
    }
    pBNOChar->setCallbacks(new BnoCharCallbacks(_bnoCbFunc));
    ESP_LOGV(TAG, "setBnoCharCallback <<"); 
}


BnoCharCallbacks::BnoCharCallbacks(BnoCharCallbackFunc_t _bnoCbFunc) : bnoCbFunc(_bnoCbFunc), TAG("BnoCharCallbacks") {
    ESP_LOGV(TAG, "constructor");
}

void BnoCharCallbacks::onWrite(BLECharacteristic* pBNOChar) {
    ESP_LOGV(TAG, ">> onWrite");
    BnoData bnoData;
    std::string data_str = pBNOChar->getValue();
    if (sizeof(BnoData) != data_str.size()) {
        ESP_LOGE(TAG, "Eroor: characteristic data size doesn't match.");
        ESP_LOGE(TAG, "expected data size:%d, char data size:%d", sizeof(BnoData), data_str.size());
        return;
    }
    memcpy(&bnoData, data_str.c_str(), sizeof(BnoData));
    if (nullptr == bnoCbFunc) {
        ESP_LOGW(TAG, "Pointer to callback func is nullptr.");
        return;
    }
    bnoCbFunc(bnoData);
    ESP_LOGV(TAG, "onWrite <<");
}