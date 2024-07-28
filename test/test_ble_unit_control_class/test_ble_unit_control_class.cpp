#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <BLEDevice.h>
#include <TFT_eSPI.h>
#include <Adafruit_BNO055.h>

const BLEUUID main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");

typedef struct {
    public:
        imu::Vector<3> Eular;
        int8_t temp;
} BnoData;

typedef struct {
    public:
        byte num_of_motor;
        short int power[4];
} MotorData;
BnoData bnoData;
MotorData motorData = {.num_of_motor = 2};
byte main_battery_level = 0;

typedef struct {
    public:
        byte command;
} Command;


//ArmService用のデータ
typedef struct {
    public:
        byte command;
        short motorPower[2];
        short servo_angle[2];
        double sensor_value[2];
}ArmData;
ArmData armData;
byte ball_count = 0;
byte arm_battery_level = 0;

//LineTracerService用のデータ
typedef struct {
    public:
        byte num_of_reflector;
        bool reflector_value[6];
        bool isIntersection;
        uint8_t Intersection_Angle[6];
} LineTracerData;
LineTracerData linetracerData;
int servo_angle = 0;
byte linetracer_battery_level = 0;


BLEClient *pClient;
BLEAdvertisedDevice *targetDevice = nullptr;
bool deviceConnected = false;
bool bMainUnit = false;
bool bLineTracerUnit = false;
bool bArmUnit = false;

class advertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    const char* TAG = "BLE Callbacks";
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        ESP_LOGI(TAG, "Found new device:, %s", advertisedDevice.toString().c_str());
        if (advertisedDevice.isAdvertisingService(main_service_uuid)) {
            ESP_LOGI(TAG, "found target device.");
            targetDevice = new BLEAdvertisedDevice(advertisedDevice);
            BLEDevice::getScan()->stop();
        }
    }
};


class MainUnitManager { //メインユニット制御用クラス　シングルトン
    public:             //マルチコア用のプログラムはまだ
        static MainUnitManager* getInstance(BLEClient* pClient, const bool& bInitialize, bool &bInitialized) {
            if (MainUnit == nullptr) {
                MainUnit = new MainUnitManager();
            }
            if (bInitialize) {
                bInitialized = MainUnit->init(pClient);
            }
            return MainUnit;
        }
        bool init(BLEClient* pClient); 
        bool getInitialized(void) const ;
        bool getMotorData(MotorData &motorData);
        bool setMotorData(const MotorData &motorData);
        bool getBnoData(BnoData &bnoData);
        bool getEular(imu::Vector<3> &vector);
        bool getTemp(int8_t &temp);
        bool getBatteryLevel(uint8_t &battery_level);
        bool sendCommand(Command command);

    private:
        static MainUnitManager* MainUnit;
        MainUnitManager(void);
        ~MainUnitManager(void);
        static const char* TAG;
        BLERemoteService *pMainService;
        BLERemoteCharacteristic *pMotorChar;
        BLERemoteCharacteristic *pBNOChar;
        BLERemoteCharacteristic *pMainBatteryLevelChar;
        static const BLEUUID main_service_uuid;
        static const BLEUUID motor_char_uuid;
        static const BLEUUID bno_char_uuid;
        static const BLEUUID main_battery_level_char_uuid;
        bool bInitialized;
};

const char* MainUnitManager::TAG = "MainUnit";
MainUnitManager* MainUnitManager::MainUnit = nullptr;
const BLEUUID MainUnitManager::main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
const BLEUUID MainUnitManager::motor_char_uuid = BLEUUID("f4c7506e-557c-4510-bc11-087c1a776155");
const BLEUUID MainUnitManager::bno_char_uuid = BLEUUID("43225596-4ab8-4493-b4f3-e065a5eeb636");
const BLEUUID MainUnitManager::main_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

MainUnitManager::MainUnitManager(void)
    : pMainService(nullptr), pMotorChar(nullptr), pBNOChar(nullptr), pMainBatteryLevelChar(nullptr), bInitialized(false) {
}

bool MainUnitManager::init(BLEClient *pClient) 
{
    pMainService = pClient->getService(main_service_uuid);
    if (nullptr == pMainService) {
        bInitialized = false;
        ESP_LOGW(TAG, "Main service was not found.");
        return false;
    }
    ESP_LOGI(TAG, "Found main service."); 
    pMotorChar = pMainService->getCharacteristic(motor_char_uuid);
    pBNOChar = pMainService->getCharacteristic(bno_char_uuid);
    pMainBatteryLevelChar = pMainService->getCharacteristic(main_battery_level_char_uuid);
    if (pMotorChar == nullptr || pBNOChar == nullptr || pMainBatteryLevelChar == nullptr) {
        bInitialized = false;
        ESP_LOGW(TAG, "Failed to find all characteristics in main service.");
        return false;
    }
    ESP_LOGI(TAG, "Successed to find all characteristics in main service.");
    bInitialized = true;
    return true;
}

bool MainUnitManager::getInitialized(void) const {
    return bInitialized;
}

bool MainUnitManager::getMotorData(MotorData &motorData) {
    ESP_LOGD(TAG, ">> getMotorData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Main unit manager is not initialized.");
        return false;
    }
    if (!pMotorChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read motor characteristic.");
        return false;
    }
    std::string data = pMotorChar->readValue();
    ESP_LOGI(TAG, "Successed to read motor characteristic.");
    if (data.size() != sizeof(motorData)) {
        ESP_LOGW(TAG, "Eroor: motor characteristic data size doesn't match.");
        return false;
    }
    memcpy(&motorData, data.c_str(), sizeof(motorData));
    ESP_LOGI(TAG, "motorData: num_of_motor=%d, power[0]=%d, power[1]=%d, power[2]=%d, power[3]=%d", motorData.num_of_motor, motorData.power[0], motorData.power[1], motorData.power[2], motorData.power[3]);
    ESP_LOGD(TAG, "getMotorData <<");
    return true;
}



bool MainUnitManager::setMotorData(const MotorData &motorData) {
    ESP_LOGD(TAG, ">> setMotorData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Main unit manager is not initialized.");
        return false;
    }
    ESP_LOGI(TAG, "motorData: num_of_motor=%d, power[0]=%d, power[1]=%d, power[2]=%d, power[3]=%d", motorData.num_of_motor, motorData.power[0], motorData.power[1], motorData.power[2], motorData.power[3]);
    uint8_t rawData[50];
    memcpy(rawData, &motorData, sizeof(motorData));
    if (!pMotorChar->canWrite()) {
        ESP_LOGW(TAG, "Eroor: Can't write value to motor characteristic.");
        return false;
    }
    pMotorChar->writeValue(rawData, sizeof(motorData), false);
    ESP_LOGI(TAG, "Successed to write value to motor characteristic.");
    ESP_LOGD(TAG, "setMotorData <<");
    return true;
}

bool MainUnitManager::getBnoData(BnoData& bnoData) {
    ESP_LOGD(TAG, ">> getBnoData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Main unit manager is not initialized.");
        return false;
    } 
    if (!pBNOChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read motor characteristic.");
        return false;
    }
    std::string data = pBNOChar->readValue();
    ESP_LOGI(TAG, "Successed to read bno characteristic.");
    if (data.size() != sizeof(bnoData)) {
        ESP_LOGW(TAG, "Eroor: bno characteristic data size doesn't match.");
        return false;
    }
    memcpy(&bnoData, data.c_str(), sizeof(bnoData));
    ESP_LOGI(TAG, "bnoData: x:%f y:%f z:%f, temp:%d", bnoData.Eular.x(), bnoData.Eular.y(), bnoData.Eular.z(), bnoData.temp);
    ESP_LOGD(TAG, "getBnoData <<");
    return true;
}


bool MainUnitManager::getTemp(int8_t &temp) {
    BnoData bnoData;
    if (!getBnoData(bnoData)) {
        return false;
    }
    temp = bnoData.temp;
    return true;
}


bool MainUnitManager::getEular(imu::Vector<3> &Eular) {
    BnoData bnoData;
    if (!getBnoData(bnoData)) {
        return false;
    }
    Eular = bnoData.Eular;
    return true;
}


bool MainUnitManager::getBatteryLevel(uint8_t &battery_level) {
    ESP_LOGD(TAG, ">> getBatteryLevel");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Main unit manager is not initialized.");
        return false;
    }
    if (!pMainBatteryLevelChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read battery level characteristic ");
        return false;
    }
    battery_level = pMainBatteryLevelChar->readUInt8();
    ESP_LOGD(TAG, "Successed to read battery characteristic.");
    ESP_LOGI(TAG, "Battery level: %d", battery_level);
    ESP_LOGD(TAG, "getBatteryLevel <<");
    return true;
}



class ArmUnitManager {
    public:
        ArmUnitManager* getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized);
        bool init(BLEClient* pClient);
        bool getInitialized(void) const;
        bool getArmData(ArmData &armData);
        bool setArmData(const ArmData& armData);
        bool getBallCount(uint8_t& ball_count);
        bool getBatteryLevel(uint8_t& arm_battery_level);
    private:
        static ArmUnitManager* armUnit;
        ArmUnitManager(void);
        ~ArmUnitManager(void);
        static const char* TAG;
        BLERemoteService *pArmService;
        BLERemoteCharacteristic *pArmChar;
        BLERemoteCharacteristic *pBallCountChar;
        BLERemoteCharacteristic *pArmBatteryLevelChar;
        static const BLEUUID arm_service_uuid;
        static const BLEUUID arm_control_char_uuid;
        static const BLEUUID ball_count_char_uuid;
        static const BLEUUID arm_battery_level_char_uuid;
        bool bInitialized;
};

ArmUnitManager* ArmUnitManager::armUnit = nullptr;
const char* ArmUnitManager::TAG = "Arm";
const BLEUUID ArmUnitManager::arm_service_uuid = BLEUUID("f5c73196-04bc-497b-b6c4-dc10a98b4d41");
const BLEUUID ArmUnitManager::arm_control_char_uuid = BLEUUID("841e98ea-cf36-43c5-be86-18c52434757c");
const BLEUUID ArmUnitManager::ball_count_char_uuid = BLEUUID("69338920-a602-424b-8992-400a0fa0187c");
const BLEUUID ArmUnitManager::arm_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

ArmUnitManager::ArmUnitManager(void)
    : pArmService(nullptr), pArmChar(nullptr), pBallCountChar(nullptr), pArmBatteryLevelChar(nullptr), bInitialized(false) {
}


bool ArmUnitManager::init(BLEClient *pClient) 
{
    pArmService = pClient->getService(main_service_uuid);
    if (nullptr == pArmService) {
        bInitialized = false;
        ESP_LOGW(TAG, "Arm service was not found.");
        return false;
    }
    ESP_LOGI(TAG, "Found arm service."); 
    pArmChar = pArmService->getCharacteristic(arm_control_char_uuid);
    pBallCountChar = pArmService->getCharacteristic(ball_count_char_uuid);
    pArmBatteryLevelChar = pArmService->getCharacteristic(arm_battery_level_char_uuid);
    if (pArmChar == nullptr || pBallCountChar == nullptr || pArmBatteryLevelChar == nullptr) {
        bInitialized = false;
        ESP_LOGW(TAG, "Failed to find all characteristics in arm service.");
        return false;
    }
    ESP_LOGI(TAG, "Successed to find all characteristics in arm service.");
    bInitialized = true;
    return true;
}

bool ArmUnitManager::getInitialized(void) const {
    return bInitialized;
}

bool ArmUnitManager::getArmData(ArmData &armData) {
    ESP_LOGD(TAG, ">> getArmData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Arm unit manager is not initialized.");
        return false;
    }
    if (!pArmChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read arm characteristic.");
        return false;
    }
    std::string data = pArmChar->readValue();
    ESP_LOGI(TAG, "Successed to read arm characteristic.");
    if (data.size() != sizeof(armData)) {
        ESP_LOGW(TAG, "Eroor: arm characteristic data size doesn't match.");
        return false;
    }
    memcpy(&armData, data.c_str(), sizeof(armData));
    ESP_LOGI(TAG, "armData: command:%d, motorPower[0]:%d, motorPower[1]:%d, servoAngle[0]:%d, servoAngle[1]:%d, sensor_value[0]:%f, sensor_value[1]:%f", armData.command, armData.motorPower[0], armData.motorPower[1],
                    armData.servo_angle[0], armData.servo_angle[1], armData.sensor_value[0], armData.sensor_value[1]);
    ESP_LOGD(TAG, "getArmData <<");
    return true;
}



bool ArmUnitManager::setArmData(const ArmData &armData) {
    ESP_LOGD(TAG, ">> setArmData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Arm unit manager is not initialized.");
        return false;
    }
    ESP_LOGI(TAG, "armData: command:%d, motorPower[0]:%d, motorPower[1]:%d, servoAngle[0]:%d, servoAngle[1]:%d, sensor_value[0]:%f, sensor_value[1]:%f", armData.command, armData.motorPower[0], armData.motorPower[1],
                    armData.servo_angle[0], armData.servo_angle[1], armData.sensor_value[0], armData.sensor_value[1]);
    uint8_t rawData[50];
    memcpy(rawData, &armData, sizeof(armData));
    if (!pArmChar->canWrite()) {
        ESP_LOGW(TAG, "Eroor: Can't write value to arm characteristic.");
        return false;
    }
    pArmChar->writeValue(rawData, sizeof(armData), false);
    ESP_LOGI(TAG, "Successed to write value to arm characteristic.");
    ESP_LOGD(TAG, "setArmData <<");
    return true;
}

bool ArmUnitManager::getBallCount(uint8_t& ball_count) {
    ESP_LOGD(TAG, ">> getBallCount");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Arm unit manager is not initialized.");
        return false;
    } 
    if (!pBallCountChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read ball count characteristic.");
        return false;
    }
    ball_count = pBallCountChar->readUInt8();
    ESP_LOGI(TAG, "Successed to read ball count characteristic.");
    ESP_LOGI(TAG, "ballCount:%d", ball_count);
    ESP_LOGD(TAG, "getBallCount <<");
    return true;
}


bool ArmUnitManager::getBatteryLevel(uint8_t &arm_battery_level) {
    ESP_LOGD(TAG, ">> getBatteryLevel");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Arm unit manager is not initialized.");
        return false;
    }
    if (!pArmBatteryLevelChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read battery level characteristic.");
        return false;
    }
    arm_battery_level = pArmBatteryLevelChar->readUInt8();
    ESP_LOGD(TAG, "Successed to read battery characteristic.");
    ESP_LOGI(TAG, "Battery level: %d", arm_battery_level);
    ESP_LOGD(TAG, "getBatteryLevel <<");
    return true;
}


class LineTracerUnitManager {
    public:
        LineTracerUnitManager* getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized);
        bool init(BLEClient* pClient);
        bool getInitialized(void) const;
        bool getData(LineTracerData &sensorData);
        bool setMode(const uint8_t& mode);
        bool getBatteryLevel(uint8_t& linetracer_battery_level);

    private:
        static LineTracerUnitManager* armUnit;
        LineTracerUnitManager(void);
        ~LineTracerUnitManager(void);
        static const char* TAG;
        BLERemoteService *pLineTracerService;
        BLERemoteCharacteristic *pDataChar;
        BLERemoteCharacteristic *pModeChar;
        BLERemoteCharacteristic *pLineTracerBatteryLevelChar;
        static const BLEUUID linetracer_service_uuid;
        static const BLEUUID data_char_uuid;
        static const BLEUUID mode_char_uuid;
        static const BLEUUID linetracer_battery_level_char_uuid;
        bool bInitialized;
};

const char* LineTracerUnitManager::TAG = "LineTracer";
const BLEUUID LineTracerUnitManager::linetracer_service_uuid = BLEUUID("d2c7abdd-7dd5-4b75-83c5-d3d972ce2963");
const BLEUUID LineTracerUnitManager::data_char_uuid = BLEUUID("4aca4700-c7de-446b-9961-f8731697b5ed");
const BLEUUID LineTracerUnitManager::mode_char_uuid = BLEUUID("b8c3d924-cb07-4f33-89e9-8024eaf30dd4");
const BLEUUID LineTracerUnitManager::linetracer_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

LineTracerUnitManager::LineTracerUnitManager(void)
    : pLineTracerService(nullptr), pDataChar(nullptr), pModeChar(nullptr), pLineTracerBatteryLevelChar(nullptr), bInitialized(false) {
}

bool LineTracerUnitManager::init(BLEClient *pClient) 
{
    pLineTracerService = pClient->getService(linetracer_service_uuid);
    if (nullptr == pLineTracerService) {
        bInitialized = false;
        ESP_LOGW(TAG, "Eroor: Linetracer service was not found.");
        return false;
    }
    ESP_LOGI(TAG, "Found linetracer service."); 
    pDataChar = pLineTracerService->getCharacteristic(data_char_uuid);
    pModeChar = pLineTracerService->getCharacteristic(mode_char_uuid);
    pLineTracerBatteryLevelChar = pLineTracerService->getCharacteristic(linetracer_battery_level_char_uuid);
    if (pDataChar == nullptr || pModeChar == nullptr || pLineTracerBatteryLevelChar == nullptr) {
        bInitialized = false;
        ESP_LOGW(TAG, "Eroor: Failed to find all characteristics in linetracer service.");
        return false;
    }
    ESP_LOGI(TAG, "Successed to find all characteristics in linetracer service.");
    bInitialized = true;
    return true;
}

bool LineTracerUnitManager::getInitialized(void) const {
    return bInitialized;
}

bool LineTracerUnitManager::getData(LineTracerData &linetracerData) {
    ESP_LOGD(TAG, ">> getData");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Linetracer unit manager is not initialized.");
        return false;
    }
    if (!pDataChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read line tracer data characteristic.");
        return false;
    }
    std::string data = pDataChar->readValue();
    ESP_LOGI(TAG, "Successed to read data characteristic.");
    if (data.size() != sizeof(linetracerData)) {
        ESP_LOGW(TAG, "Eroor: Linetracer characteristic data size doesn't match.");
        return false;
    }
    memcpy(&linetracerData, data.c_str(), sizeof(linetracerData));
    ESP_LOGI(TAG, "linetracerData: num_of_reflector:%d, raw value:%d,%d,%d,%d,%d,%d isIntersection=%d", 
                linetracerData.num_of_reflector,
                linetracerData.reflector_value[0], linetracerData.reflector_value[1],
                linetracerData.reflector_value[2], linetracerData.reflector_value[3],
                linetracerData.reflector_value[4], linetracerData.reflector_value[5],
                linetracerData.isIntersection);
    ESP_LOGD(TAG, "getData <<");
    return true;
}


bool LineTracerUnitManager::setMode(const uint8_t& mode) {
    ESP_LOGD(TAG, ">> setMode");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Linetracer unit manager is not initialized.");
        return false;
    }
    ESP_LOGI(TAG, "mode:%d", mode);
    if (!pModeChar->canWrite()) {
        ESP_LOGW(TAG, "Eroor: Can't write value to command characteristic.");
        return false;
    }
    pModeChar->writeValue(mode);
    ESP_LOGI(TAG, "Successed to write value to command characteristic.");
    ESP_LOGD(TAG, "setMode <<");
    return true;
}

bool LineTracerUnitManager::getBatteryLevel(uint8_t &linetracer_battery_level) {
    ESP_LOGD(TAG, ">> getBatteryLevel");
    if (!bInitialized) {
        ESP_LOGW(TAG, "Eroor: Arm unit manager is not initialized.");
        return false;
    }
    if (!pLineTracerBatteryLevelChar->canRead()) {
        ESP_LOGW(TAG, "Eroor: Can't read battery level characteristic.");
        return false;
    }
    linetracer_battery_level = pLineTracerBatteryLevelChar->readUInt8();
    ESP_LOGD(TAG, "Successed to read battery characteristic.");
    ESP_LOGI(TAG, "Battery level: %d", linetracer_battery_level);
    ESP_LOGD(TAG, "getBatteryLevel <<");
    return true;
}




MainUnitManager* mainUnit;

void setup(void) {
    const char* TAG = "setup";
    Serial.begin(115200);
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    BLEScan *pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new advertisedDeviceCallbacks());
    pScan->setActiveScan(true);
    pScan->start(5, false);
    if (targetDevice == nullptr) {
        ESP_LOGE(TAG, "couldn't find target device.");
    }
    else {
        if (pClient->connect(targetDevice)) {
            ESP_LOGI(TAG, "Client connected to target device.");
            deviceConnected = true;
        }
        else {
            ESP_LOGW(TAG, "Failed to connect to target device.");
        }
    }

    if (deviceConnected) { //サービスおよび特性のチェック
        bool bInitialized = false;
        mainUnit = MainUnitManager::getInstance(pClient, true, bInitialized);
        //mainUnit->init(pClient);
        mainUnit->getMotorData(motorData);
        motorData.power[0] = 255; motorData.power[1] = -255;
        mainUnit->setMotorData(motorData);
        imu::Vector<3> Eular;
        int8_t temp;
        mainUnit->getEular(Eular);
        ESP_LOGI(TAG, "Eular: x:%f y:%f z:%f", Eular.x(), Eular.y(), Eular.z());
        mainUnit->getTemp(temp);
        ESP_LOGI(TAG, "temp:%d", temp);
        mainUnit->getBatteryLevel(main_battery_level);
        ESP_LOGI(TAG, "Battery level:%d", main_battery_level);
    }
}

void loop(void) {
    if (deviceConnected && mainUnit->getInitialized()) {
        mainUnit->getBnoData(bnoData);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}