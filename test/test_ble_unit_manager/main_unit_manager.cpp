#include "unit_manager.hpp"
#include "main_unit_manager.hpp"

MainUnitManager* MainUnitManager::mainUnit = nullptr;

const BLEUUID MainUnitManager::main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
const BLEUUID MainUnitManager::motor_char_uuid = BLEUUID("f4c7506e-557c-4510-bc11-087c1a776155");
const BLEUUID MainUnitManager::bno_char_uuid = BLEUUID("43225596-4ab8-4493-b4f3-e065a5eeb636");
const BLEUUID MainUnitManager::main_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

MainUnitManager::MainUnitManager(void) : UnitManager(main_service_uuid), pMotorChar(nullptr), pBNOChar(nullptr)
{
    strcpy(TAG, "MainUnit");
    ESP_LOGD(TAG, "MainUnitManager()\n");
}


MainUnitManager* MainUnitManager::getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized) {
    ESP_LOGD(TAG, ">> getInstance");
    if (nullptr == mainUnit) {
        mainUnit = new MainUnitManager();
    }
    if (bInitialize) {
        mainUnit->init(pClient);
    }
    bInitialized = mainUnit->bInitialized;
    ESP_LOGD(TAG, "getInstance <<\n");
    return mainUnit;
}


bool MainUnitManager::init(BLEClient* pClient) {
    ESP_LOGD(TAG, ">> init");
    if (InitService(pClient)) {
        pMotorChar = pService->getCharacteristic(motor_char_uuid);
        pBNOChar = pService->getCharacteristic(bno_char_uuid);
        if (nullptr != pMotorChar && nullptr != pBNOChar) {
            bInitialized = true;
        }
    }
    ESP_LOGD(TAG, "init <<");
    return bInitialized;
}

bool MainUnitManager::getMotorData(MotorData &motorData) {
    ESP_LOGD(TAG, ">> getMotorData");
    uint8_t data[50];
    bool result = getData(pMotorChar, data, sizeof(motorData));
    memcpy(&motorData, data, sizeof(motorData));
    ESP_LOGI(TAG, "motorData num_of_motor:%d, power[0]:%d, power[1]:%d, power[2]:%d, power[3]:%d", 
            motorData.num_of_motor, motorData.power[0], motorData.power[1], motorData.power[2], motorData.power[3]);
    ESP_LOGD(TAG, "getMotorData <<\n");
    return result;
}

bool MainUnitManager::setMotorData(const MotorData &motorData) {
    ESP_LOGD(TAG, ">> setMotorData");
    ESP_LOGI(TAG, "motorData num_of_motor:%d, power[0]:%d, power[1]:%d, power[2]:%d, power[3]:%d", 
            motorData.num_of_motor, motorData.power[0], motorData.power[1], motorData.power[2], motorData.power[3]);
    uint8_t data[50];
    memcpy(data, &motorData, sizeof(motorData));
    bool result = setData(pMotorChar, data, sizeof(motorData));
    ESP_LOGD(TAG, "setMotorData <<\n");
    return result;
}

bool MainUnitManager::getBnoData(BnoData& bnoData) {
    ESP_LOGD(TAG, ">> getBnoData");
    uint8_t data[100];
    bool result = getData(pBNOChar, data, sizeof(bnoData));
    memcpy(&bnoData, data, sizeof(bnoData));
    ESP_LOGI(TAG, "bnoData: Eular.x():%f, Eular.y():%f, Eular.z():%f temp:%d", 
            bnoData.Eular.x(), bnoData.Eular.y(), bnoData.Eular.z(), bnoData.temp);
    ESP_LOGD(TAG, "getBnoData <<");
    return result;
}

bool MainUnitManager::getEular(imu::Vector<3> &eular) {
    ESP_LOGD(TAG, ">> getEular");
    BnoData bnoData;
    bool result = getBnoData(bnoData);
    eular = bnoData.Eular;
    ESP_LOGD(TAG, "getEular <<\n");
    return result;
}

bool MainUnitManager::getTemp(int8_t &temp) {
    ESP_LOGD(TAG, ">> getTemp");   
    BnoData bnoData;
    bool result = getBnoData(bnoData);
    temp = bnoData.temp;
    ESP_LOGD(TAG, "getTemp <<\n");  
    return result; 
}