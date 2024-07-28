#include <Arduino.h>
#include <motor.hpp>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <BLEDevice.h>


Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28, &Wire);

//BLE reference
// https://lang-ship.com/reference/unofficial/M5StickC/Class/ESP32/BLEDevice/ 

//uuid generator
// https://www.uuidgenerator.net/ 

bool bLineTracerConnection = true;
bool bArmConnection = true;

//MainService用のデータ
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
typedef struct {
    public:
        byte num_of_reflector;
        byte bValue;
        short iValue[8];
        bool isInterSection;
        uint8_t Intersection_Angle[6];
} LineData;
LineData lineData;
byte linetracer_battery_level = 75;


//ArmService用のデータ
typedef struct {
    public:
        short motorPower[2];
        short servo_angle[2];
        double sensor_value[2];
} ArmData;

ArmData armData;
byte ball_count = 7;
byte arm_battery_level = 70;

BLEServer *pServer;


//共通
BLEUUID name_char_uuid = BLEUUID((uint16_t)0x2A00);
BLEUUID battery_level_char_uuid = BLEUUID("00002A19-0000-1000-8000-00805F9B34FB");


BLEService *pMainService;
BLECharacteristic *pMainNameChar;
BLECharacteristic *pMotorChar;
BLECharacteristic *pBNOChar;
BLECharacteristic *pMainBatteryLevelChar;
BLEUUID main_service_uuid = BLEUUID("14998279-068a-4b45-a25a-812544e707e0");
BLEUUID motor_char_uuid = BLEUUID("f4c7506e-557c-4510-bc11-087c1a776155");
BLEUUID bno_char_uuid = BLEUUID("43225596-4ab8-4493-b4f3-e065a5eeb636");

BLEService *pArmService;
BLECharacteristic *pArmNameChar;
BLECharacteristic *pArmChar;
BLECharacteristic *pBallCountChar;
BLECharacteristic *pArmBatteryLevelChar;
BLEUUID arm_service_uuid = BLEUUID("f5c73196-04bc-497b-b6c4-dc10a98b4d41");
BLEUUID arm_control_char_uuid = BLEUUID("841e98ea-cf36-43c5-be86-18c52434757c");
BLEUUID ball_count_char_uuid = BLEUUID("69338920-a602-424b-8992-400a0fa0187c");

BLEService *pLineTracerService;
BLECharacteristic *pLineTracerNameChar;
BLECharacteristic *pReflectorChar;
BLECharacteristic *pLineTracerBatteryLevelChar;
BLEUUID linetracer_service_uuid = BLEUUID("d2c7abdd-7dd5-4b75-83c5-d3d972ce2963");
BLEUUID reflector_char_uuid = BLEUUID("4aca4700-c7de-446b-9961-f8731697b5ed");

Motor motor;
bool connected = false;

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        log_i("Device connected");
        BLEDevice::stopAdvertising();
        connected = true;
    }
    void onDisconnect(BLEServer *pServer) {
        log_i("Device disconnected");
        BLEDevice::startAdvertising();
        connected = false;
    }
};


class CharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        uint8_t *data = pCharacteristic->getData();
        size_t length = pCharacteristic->getLength();
        if (pCharacteristic->getUUID().equals(motor_char_uuid)) {
            log_i("Motor characteristic was rewritten.");
            memcpy(&motorData, data, sizeof(motorData));
        }
        else if (pCharacteristic->getUUID().equals(arm_control_char_uuid)) {
            log_i("Arm control characteristic was rewritten");
            memcpy(&armData, data, sizeof(armData));
        }
        else {
            log_i("Another characteristic was rewritten.");
        }
        ESP_LOG_BUFFER_HEX("value", data, length);
    }
    void onRead(BLECharacteristic* pCharacteristic) {
        if (pCharacteristic->getUUID().equals(motor_char_uuid)) {
            log_i("Motor characteristic was read.");
        }
        else if (pCharacteristic->getUUID().equals(arm_control_char_uuid)) {
            log_i("Arm control characteristic was read");
        }
        else {
            log_i("Another characteristic was read.");
        }
    }
};


void InitMainService(std::string Name) {
    uint8_t data[100];

    pMainService = pServer->createService(main_service_uuid);

    //Characteristic settings
    //Name
    pMainNameChar = pMainService->createCharacteristic(name_char_uuid, BLECharacteristic::PROPERTY_READ);
    pMainNameChar->setValue(Name);

    //Motor
    pMotorChar = pMainService->createCharacteristic(motor_char_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    memcpy(data, &motorData, sizeof(motorData));
    pMotorChar->setValue(data, sizeof(motorData));
    pMotorChar->setCallbacks(new CharacteristicCallbacks); //コールバック関数のセット

    //BNO
    pBNOChar = pMainService->createCharacteristic(bno_char_uuid, BLECharacteristic::PROPERTY_READ);
    memcpy(data, &bnoData, sizeof(bnoData));
    pBNOChar->setValue(data, sizeof(bnoData));

    //Battery
    pMainBatteryLevelChar = pMainService->createCharacteristic(battery_level_char_uuid, BLECharacteristic::PROPERTY_READ);
    pMainBatteryLevelChar->setValue(&main_battery_level, 1);

    pMainService->start();
    log_i("Initialized main service.");
}

void InitArmService(std::string Name) {
    uint8_t data[100];

    pArmService = pServer->createService(arm_service_uuid);

    //Characteristic settings
    //Arm Control
    pArmNameChar = pArmService->createCharacteristic(name_char_uuid, BLECharacteristic::PROPERTY_READ);
    pArmNameChar->setValue(Name);

    pArmChar = pArmService->createCharacteristic(arm_control_char_uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    memcpy(data, &armData, sizeof(armData));
    pArmChar->setValue(data, sizeof(armData));
    pArmChar->setCallbacks(new CharacteristicCallbacks); //コールバック関数のセット

    //Ball count
    pBallCountChar = pArmService->createCharacteristic(ball_count_char_uuid, BLECharacteristic::PROPERTY_READ);
    pBallCountChar->setValue(&ball_count, 1);

    //Arm Battery Level
    pArmBatteryLevelChar = pArmService->createCharacteristic(battery_level_char_uuid, BLECharacteristic::PROPERTY_READ);
    pArmBatteryLevelChar->setValue(&arm_battery_level, 1);

    pArmService->start();
    log_i("Initialized arm service.");
}

void InitLineTracerService(std::string Name) {
    uint8_t data[100];
    pLineTracerService = pServer->createService(linetracer_service_uuid);

    //Characteristic settings
    pLineTracerNameChar = pLineTracerService->createCharacteristic(name_char_uuid, BLECharacteristic::PROPERTY_READ);
    pLineTracerNameChar->setValue(Name);

    //Photo Reflector
    pReflectorChar = pLineTracerService->createCharacteristic(reflector_char_uuid, BLECharacteristic::PROPERTY_READ);
    memcpy(data, &lineData, sizeof(lineData));
    pReflectorChar->setValue(data, sizeof(lineData));

    //Battery Level
    pLineTracerBatteryLevelChar = pLineTracerService->createCharacteristic(battery_level_char_uuid, BLECharacteristic::PROPERTY_READ);
    pLineTracerBatteryLevelChar->setValue(&linetracer_battery_level, 1);

    pLineTracerService->start();
    log_i("Initialized line tracer service.");
}


void InitBLEServer(void) {
    BLEDevice::init("VICTORY");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    InitMainService("BIG BEAR REVIVED");
    if (bArmConnection) {
        log_i("Arm is connected");
        InitArmService("HUNTER");
    }
    if (bLineTracerConnection) {
        log_i("Line tracer is connected");
        InitLineTracerService("EDGE FINDER");
    }
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    //BLEAdvertisementData oAdvertisementData;
    pAdvertising->addServiceUUID(main_service_uuid);
    BLEDevice::startAdvertising();
}


void setup(void) 
{
    Serial.begin(115200);
    while (!Serial) vTaskDelay(pdMS_TO_TICKS(10));
    log_i("Sensor Raw Data Test");
    if (!bno.begin()) {
        log_i("No BNO055 detected");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    log_i("BNO055 detected!"); 
    vTaskDelay(pdMS_TO_TICKS(1000));
    bnoData.temp = bno.getTemp();
    log_i("temp: %dC", bnoData.temp);
    bno.setExtCrystalUse(true);
    
    motorData.num_of_motor = 2;//テスト用
    motorData.power[0] = 128;  
    motorData.power[1] = -100;
    lineData.num_of_reflector = 5;
    motor.attach(16, 17);
    InitBLEServer();
}



void loop(void) {
    uint8_t system, gyro, accel, mag = 0;
    bno.getCalibration(&system, &gyro, &accel, &mag);
    // Serial.print(">CALIBRATION_Sys:");
    // Serial.println(system, DEC);
    // Serial.print(">Gyro:");
    // Serial.println(gyro, DEC);
    // Serial.print(">Accel:");
    // Serial.println(accel, DEC);
    // Serial.print(">Mag:");
    // Serial.println(mag, DEC);
    adafruit_bno055_opmode_t opmode = bno.getMode();
    bnoData.Eular = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    if (connected) {
        motor.drive(motorData.power[0]);
        pBNOChar->setValue((uint8_t*)&bnoData, sizeof(bnoData));
        static bool bLineTracerService = true;
        static bool bArmService = true;
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == 'l') {
                bLineTracerService = !bLineTracerService;
                if (bLineTracerService) {
                    pLineTracerService->start();
                    log_i("LineTracer Service started.");
                } 
                else {
                    pLineTracerService->stop();
                    log_i("LineTracer Service stopped.");
                }
            }
            else if (c == 'a') {
                bArmService = !bArmService;
                if (bArmService) {
                    pArmService->start();
                    log_i("Arm Service started.");
                }
                else {
                    pArmService->stop();
                    log_i("Arm Service stopped.");
                }
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}
