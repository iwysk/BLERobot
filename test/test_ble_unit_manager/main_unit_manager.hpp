#ifndef MAIN_UNIT_MANAGER
#define MAIN_UNIT_MANAGER

#include <Arduino.h>
#include <BLEDevice.h>
#include <Adafruit_BNO055.h>
#include "unit_manager.hpp"

//メインユニット用
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


class MainUnitManager final : public UnitManager {
    public:
        static MainUnitManager* getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized);
        bool init(BLEClient* pClient) override;
        bool getMotorData(MotorData &motorData);
        bool setMotorData(const MotorData &motorData);
        bool getBnoData(BnoData &bnoData);
        bool getEular(imu::Vector<3> &eular);
        bool getTemp(int8_t &temp);

    private:
        MainUnitManager(void);
        ~MainUnitManager(void);
        static MainUnitManager* mainUnit;
        BLERemoteCharacteristic* pMotorChar;
        BLERemoteCharacteristic* pBNOChar;
        static const BLEUUID main_service_uuid;
        static const BLEUUID motor_char_uuid;
        static const BLEUUID bno_char_uuid;
        static const BLEUUID main_battery_level_char_uuid;
};

#endif
