#ifndef ARM_SERVICE_MANAGER
#define ARM_SERVICE_MANAGER
#include "service_manager.hpp"

extern const BLEUUID arm_service_uuid;
extern const BLEUUID arm_char_uuid;
extern const BLEUUID ball_count_char_uuid;

typedef struct {
    public:
        short motorPower[2];
        short servo_angle[2];
        double sensor_value[2];
} ArmData;

class ArmServiceManager final : public ServiceManager {
    public:
        static ArmServiceManager* getInstance(void);
        bool init(BLEClient* _pClient) override;
        bool getArmData(ArmData &armData);
        bool setArmData(const ArmData& armData);
        bool getBallCount(uint8_t& ball_count);

    private:
        static ArmServiceManager* arm;
        ArmServiceManager(void);
        ~ArmServiceManager(void);
        BLERemoteCharacteristic *pArmChar;
        BLERemoteCharacteristic *pBallCountChar;
};

#endif