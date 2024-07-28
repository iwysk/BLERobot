//メインユニットBLEService担当プログラム　GPIOやLED通信までは干渉しない
//あとでcreateBNOTaskは削除していいかもしれない　実際わからない

#ifndef MAIN
#define MAIN

#include <Arduino.h>
#include <Adafruit_BNO055.h>
#include "base_service.hpp"


extern BLEUUID main_service_uuid;
extern BLEUUID motor_char_uuid;
extern BLEUUID bno_char_uuid;


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

// typedef void (*MotorCharCallbackFunc_t)(MotorData);

// class MotorCharCallbacks final : public BLECharacteristicCallbacks {
//     public:
//         MotorCharCallbacks(void) = delete;
//         MotorCharCallbacks(MotorCharCallbackFunc_t _motorCbFunc);
//         void onWrite(BLECharacteristic* pMotorChar) override;
//     private:
//         MotorCharCallbackFunc_t motorCbFunc;
//         static const char *TAG;
// };

typedef void (*BnoCharCallbackFunc_t)(const BnoData&);

class MainService final : public BaseService {
    public:
        static MainService* getInstance(void);
        void init(BLEServer* pServer) override;
        bool setMotorData(const MotorData& motorData);
        void getBnoData(BnoData& bnoData);
        void setBnoCharCallback(BnoCharCallbackFunc_t _bnoCbFunc);
        
    private:
        MainService(void);
        ~MainService(void);
        static MainService* mainService; 
        BLECharacteristic* pMotorChar;
        BLECharacteristic* pBNOChar;
};


class BnoCharCallbacks final : public BLECharacteristicCallbacks {
    public:
        BnoCharCallbacks(void) = delete;
        BnoCharCallbacks(BnoCharCallbackFunc_t _bnoCbFunc);
        void onWrite(BLECharacteristic* pMotorChar) override;
    private:
        BnoCharCallbackFunc_t bnoCbFunc;
        const char *TAG;
};

#endif