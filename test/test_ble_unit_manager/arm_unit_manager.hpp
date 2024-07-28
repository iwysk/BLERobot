#ifndef ARM_UNIT_MANAGER
#define ARM_UNIT_MANAGER

#include <Arduino.h>
#include <BLEDevice.h>
#include <unit_manager.hpp>

typedef struct {
    public:
        short motorPower[2];
        short servo_angle[2];
        double sensor_value[2];
} ArmData;

class ArmUnitManager : public UnitManager {
    public:
        static ArmUnitManager* getInstance(BLEClient* pClient, const bool& bIntialize, bool& bInitialized);
        bool init(BLEClient* pClient) override;
        bool getArmData(ArmData &armData);
        bool setArmData(const ArmData& armData);
        bool getBallCount(uint8_t& ball_count);

    private:
        static ArmUnitManager* armUnit;
        ArmUnitManager(void);
        ~ArmUnitManager(void);
        BLERemoteCharacteristic *pArmChar;
        BLERemoteCharacteristic *pBallCountChar;
        static const BLEUUID arm_service_uuid;
        static const BLEUUID arm_char_uuid;
        static const BLEUUID ball_count_char_uuid;
};

#endif