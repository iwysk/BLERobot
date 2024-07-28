#ifndef MAIN_SERVICE_MANAGER
#define MAIN_SERVICE_MANAGER

#include <Arduino.h>
#include <BLEDevice.h>
#include <Adafruit_BNO055.h>
#include "service_manager.hpp"

extern const BLEUUID main_service_uuid;
extern const BLEUUID motor_char_uuid;
extern const BLEUUID bno_char_uuid;


struct BnoData {
    public:
        float eular[3];
        int8_t temp;
};


struct MotorData {
    public:
        byte num_of_motor;
        short int power[4];
        bool operator==(const MotorData& motorData) const;
        bool operator!=(const MotorData& motorData) const;
};

typedef void (*MotorCharCallbackFunc_t)(const MotorData&);
static MotorCharCallbackFunc_t MotorCbFunc;

class MainServiceManager final : public ServiceManager {
    public:
        static MainServiceManager* getInstance(void);
        bool init(BLEClient* _pClient) override;
        bool getMotorData(MotorData &motorData);
        bool setMotorCallback(MotorCharCallbackFunc_t MotorCbFunc);
        bool setBnoData(const BnoData &bnoData);

    private:
        MainServiceManager(void);
        ~MainServiceManager(void);
        static MainServiceManager* main;
        BLERemoteCharacteristic* pMotorChar;
        BLERemoteCharacteristic* pBNOChar;
};

void notifyCallbackForMotorChar(BLERemoteCharacteristic* pMotorChar, uint8_t* data, size_t length, bool isNotify);


#endif
