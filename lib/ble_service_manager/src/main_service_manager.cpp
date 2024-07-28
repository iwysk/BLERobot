#include "main_service_manager.hpp"

const BLEUUID main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
const BLEUUID motor_char_uuid = BLEUUID("f4c7506e-557c-4510-bc11-087c1a776155");
const BLEUUID bno_char_uuid = BLEUUID("43225596-4ab8-4493-b4f3-e065a5eeb636");

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

MainServiceManager* MainServiceManager::main = nullptr;

MainServiceManager::MainServiceManager(void) : ServiceManager(main_service_uuid), pMotorChar(nullptr), pBNOChar(nullptr)
{
    strcpy(TAG, "MainUnit");
    esp_log_level_set(TAG, ESP_LOG_WARN);
    ESP_LOGV(TAG, "MainServiceManager()");
}


MainServiceManager* MainServiceManager::getInstance(void) {
    ESP_LOGV("MainServiceManager", ">> getInstance");
    if (nullptr == main) {
        main = new MainServiceManager();
    }
    ESP_LOGV("MainServiceManager", "getInstance <<");
    return main;
}


bool MainServiceManager::init(BLEClient* _pClient) {
    ESP_LOGV(TAG, ">> init");
    pClient = _pClient;
    if (InitService()) {
        pMotorChar = pService->getCharacteristic(motor_char_uuid);
        pBNOChar = pService->getCharacteristic(bno_char_uuid);
        if (nullptr != pMotorChar && nullptr != pBNOChar) {
            bInitialized = true;
        }
    }
    ESP_LOGV(TAG, "init <<");
    return bInitialized;
}


bool MainServiceManager::getMotorData(MotorData &motorData) {
    ESP_LOGV(TAG, ">> getMotorData");
    uint8_t data[sizeof(MotorData)];
    if (!getData(pMotorChar, data, sizeof(MotorData))) {
        return false;
    }
    memcpy(data, &motorData, sizeof(MotorData));
    ESP_LOGD(TAG, "motorData num_of_motor:%d, power[0]:%d, power[1]:%d, power[2]:%d, power[3]:%d", 
            motorData.num_of_motor, motorData.power[0], motorData.power[1], motorData.power[2], motorData.power[3]);
    ESP_LOGV(TAG, "getMotorData <<");
    return true;
}

bool MainServiceManager::setMotorCallback(MotorCharCallbackFunc_t _MotorCbFunc) {
    ESP_LOGV(TAG, ">> setMotorCallback");
    if (!bInitialized) {
        ESP_LOGE(TAG, "Eroor: Service is not initialized.");
        ESP_LOGV(TAG, "setMotorCallback <<");
        return false;
    }
    if (!pClient->isConnected()) {
        ESP_LOGE(TAG, "Eroor: Client is not connected.");
        ESP_LOGV(TAG, "setMotorCallback <<");
        return false;
    }
    MotorCbFunc = _MotorCbFunc;
    pMotorChar->registerForNotify(notifyCallbackForMotorChar);
    ESP_LOGV(TAG, "setMotorCallback <<");
    return true;
}

bool MainServiceManager::setBnoData(const BnoData& bnoData) {
    ESP_LOGV(TAG, ">> setBnoData");
    ESP_LOGD(TAG, "bnoData: eular.x():%f, eular.y():%f, eular.z():%f temp:%d", 
            bnoData.eular.x(), bnoData.eular.y(), bnoData.eular.z(), bnoData.temp);
    uint8_t data[sizeof(BnoData)];
    memcpy(data, &bnoData, sizeof(BnoData));
    bool result = setData(pBNOChar, data, sizeof(BnoData));
    ESP_LOGV(TAG, "setBnoData <<");
    return result;
}


void notifyCallbackForMotorChar(BLERemoteCharacteristic *pMotorChar, uint8_t* data, size_t length, bool isNotify) {
    const char* TAG = "Notify";
    ESP_LOGV(TAG, ">> notifyCallbackForMotorChar");
    if (sizeof(MotorData) != length) {
        ESP_LOGE(TAG, "Charactertistic size doesn't match.");
        ESP_LOGE(TAG, "Expected data size: %d, Char data size: %d", sizeof(MotorData), length);
        ESP_LOGV(TAG, "notifyCallbackForMotorChar <<");
        return;
    }
    MotorData motorData;
    memcpy(&motorData, data, sizeof(MotorData));
    MotorCbFunc(motorData);
    ESP_LOGV(TAG, "notifyCallbackForMotorChar <<");
}