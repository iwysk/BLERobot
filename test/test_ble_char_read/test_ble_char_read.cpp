#include <Arduino.h>
#include <BLEDevice.h>
#include <TFT_eSPI.h>
#include <Adafruit_BNO055.h>

struct BnoData {
    public:
        imu::Vector<3> Eular;
        int8_t temp;
};
struct MotorData {
    public:
        byte num_of_motor;
        short int power[4];
};
struct BnoData bnoData;
struct MotorData motorData;
byte main_battery_level = 80;


//LineTracerService用のデータ
struct reflectorRawData {
    public:
        byte num_of_reflector;
        bool reflector_value[6];
};
struct reflectorRawData refRawData;
int servo_angle = 90;
byte linetracer_battery_level = 75;


//LineTracerService用のデータ
struct ArmData {
    public:
        int motorDirection[4];
        int servo_angle[4];
        double sensor_value[4];
};
struct ArmData armData;
byte ball_count = 0;
byte arm_battery_level = 70;

BLEClient *pClient;

BLERemoteService *pMainService;
BLERemoteCharacteristic *pMotorChar;
BLERemoteCharacteristic *pBNOChar;
BLERemoteCharacteristic *pMainBatteryLevelChar;
BLEUUID main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
BLEUUID motor_char_uuid = BLEUUID("f4c7506e-557c-4510-bc11-087c1a776155");
BLEUUID bno_char_uuid = BLEUUID("43225596-4ab8-4493-b4f3-e065a5eeb636");
BLEUUID main_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

BLERemoteService *pLineTracerService;
BLERemoteCharacteristic *pReflectorChar;
BLERemoteCharacteristic *pServoChar;
BLERemoteCharacteristic *pLineTracerBatteryLevelChar;
BLEUUID linetracer_service_uuid = BLEUUID("d2c7abdd-7dd5-4b75-83c5-d3d972ce2963");
BLEUUID reflector_char_uuid = BLEUUID("4aca4700-c7de-446b-9961-f8731697b5ed");
BLEUUID servo_char_uuid = BLEUUID("b8c3d924-cb07-4f33-89e9-8024eaf30dd4");
BLEUUID linetracer_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");

BLERemoteService *pArmService;
BLERemoteCharacteristic *pArmControlChar;
BLERemoteCharacteristic *pBallCountChar;
BLERemoteCharacteristic *pArmBatteryLevelChar;
BLEUUID arm_service_uuid = BLEUUID("f5c73196-04bc-497b-b6c4-dc10a98b4d41");
BLEUUID arm_control_char_uuid = BLEUUID("841e98ea-cf36-43c5-be86-18c52434757c");
BLEUUID ball_count_char_uuid = BLEUUID("69338920-a602-424b-8992-400a0fa0187c");
BLEUUID arm_battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");


BLEAdvertisedDevice *targetDevice = NULL;
bool deviceConnected = false;
bool bMainUnit = false;
bool bLineTracerUnit = false;
bool bArmUnit = false;



class advertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        log_i("Found new device:, %s", advertisedDevice.toString().c_str());
        if (advertisedDevice.isAdvertisingService(main_service_uuid)) {
            log_i("found target device.");
            targetDevice = new BLEAdvertisedDevice(advertisedDevice);
            BLEDevice::getScan()->stop();
        }
    }
};





void BLEServerCheck(void) {
    pMainService = pClient->getService(main_service_uuid);
    bMainUnit = (NULL != pMainService);
    bMainUnit ? log_i("Found main service.") : log_i("Main service was not found.");
    if (!bMainUnit) {
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    pLineTracerService = pClient->getService(linetracer_service_uuid);
    bLineTracerUnit = (NULL != pLineTracerService);
    bLineTracerUnit ? log_i("Found linetrace service.") : log_i("Linetrace service was not found.");

    pArmService = pClient->getService(arm_service_uuid);
    bArmUnit = (NULL != pArmService);
    bArmUnit ? log_i("Found arm service.") : log_i("Arm service was not found.");

    if (bMainUnit) {
        pMotorChar = pMainService->getCharacteristic(motor_char_uuid);
        pBNOChar = pMainService->getCharacteristic(bno_char_uuid);
        pMainBatteryLevelChar = pMainService->getCharacteristic(main_battery_level_char_uuid);
        if (pMotorChar == NULL || pBNOChar == NULL || pMainBatteryLevelChar == NULL) {
            log_i("Failed to find all characteristics in main service.");
            bMainUnit = false;
        }
        else {
            log_i("Successed to find all characteristics in main service.");
        }
    }
    if (bLineTracerUnit) {
        pReflectorChar = pLineTracerService->getCharacteristic(reflector_char_uuid);
        pServoChar = pLineTracerService->getCharacteristic(servo_char_uuid);
        pArmBatteryLevelChar = pLineTracerService->getCharacteristic(arm_battery_level_char_uuid);
        if (pReflectorChar == NULL || pServoChar == NULL || pArmBatteryLevelChar == NULL) {
            log_i("Failed to find all characteristics in linetracer service.");
            bLineTracerUnit = false;
        }
        else {
            log_i("Successed to find all characteristics in linetracer service.");
        }
    }
    if (bArmUnit) {
        pArmControlChar = pArmService->getCharacteristic(arm_control_char_uuid);
        pBallCountChar = pArmService->getCharacteristic(ball_count_char_uuid);
        pArmBatteryLevelChar = pArmService->getCharacteristic(arm_battery_level_char_uuid);
        if (pArmControlChar == NULL || pBallCountChar == NULL || pArmBatteryLevelChar == NULL) {
            log_i("Failed to find all characteristics in arm service.");
            bArmUnit = false;
        }
        else {
            log_i("Successed to find all characteristics in arm service.");
        }
    }
}




void setup(void) {
    Serial.begin(115200);
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    BLEScan *pScan = BLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new advertisedDeviceCallbacks());
    pScan->setActiveScan(true);
    pScan->start(5, false);
    if (targetDevice == NULL) {
        log_i("couldn't find target device.");
    }
    else {
        if (pClient->connect(targetDevice)) {
            log_i("Client connected to target device.");
            deviceConnected = true;
        }
        else {
            log_i("Failed to connect target device.");
        }
    }

    if (deviceConnected) { //サービスおよび特性のチェック
        BLEServerCheck();
    }
}

void loop(void) {
    
}