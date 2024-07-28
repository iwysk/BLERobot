//アームユニットBLEService担当プログラム　GPIOやLED通信までは干渉しない
//あとでcreateBNOTaskは削除していいかもしれない　実際わからない

#ifndef ARM
#define ARM
#include <Arduino.h>
#include "base_service.hpp"

extern BLEUUID arm_service_uuid;
extern BLEUUID arm_char_uuid;
extern BLEUUID ball_count_char_uuid;

typedef void (*BallCountCharCallbackFunc_t)(const uint8_t&);

struct ArmData {
    public:
        short motorPower[2];
        short servo_angle[2];
        double sensor_value[2];
};

class ArmService final : public BaseService {
    public:
        static ArmService* getInstance(void);
        void init(BLEServer* pServer) override;
        bool getArmData(ArmData &armData);
        void setArmdata(const ArmData& armData);
        void getBallCount(uint8_t& ball_count);
        void setBallCountCharCallback(BallCountCharCallbackFunc_t _ballCountCbFunc);

    private:
        ArmService(void);
        ~ArmService(void);
        static ArmService* arm;
        BLECharacteristic *pArmChar;
        BLECharacteristic *pBallCountChar;
};


class BallCountCharCallbacks : public BLECharacteristicCallbacks {
    public:
        BallCountCharCallbacks(void) = delete;
        BallCountCharCallbacks(BallCountCharCallbackFunc_t _ballCountCbFunc);
        void onWrite(BLECharacteristic* pBallCountChar) override;
    private:
        BallCountCharCallbackFunc_t ballCountCbFunc;
        const char* TAG;
};
#endif