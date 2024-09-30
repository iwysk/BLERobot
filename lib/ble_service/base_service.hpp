#ifndef BASE
#define BASE

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2902.h>

//変えたい点　initServiceをもっと別の名前にする
//CommandCharをクライアント側にも
//そもそもUnitManagerとかBaseServiceとかいう表現が微妙　サーバーサイドとクライアントサイド逆の方がいいまである

extern BLEUUID name_char_uuid;
extern BLEUUID battery_level_char_uuid;
extern BLEUUID command_char_uuid;

struct Command {
    public:
        uint8_t command;
        int parameter;
};

typedef void (*BatteryLevelCharCallbackFunc_t)(const char*, const uint8_t&);


class BaseService { //共通項
    public:
        virtual void init(BLEServer* pServer) = 0;
        bool getInitialized(void) const;
        void startService(void);
        std::string getName(void);
        void getBatteryLevel(uint8_t& battery_level);
        void setBatteryLevelCharCallback(BatteryLevelCharCallbackFunc_t _batteryLevelCbFunc);
        void setCommand(const Command& command);
        void getCommand(Command& command);
        bool isActivated(void) const;
        virtual void cleanUp(void);

    protected:
        BaseService(void) = delete;
        BaseService(BLEUUID _service_uuid);
        void initService(BLEServer* pServer);
        bool getData(BLECharacteristic* &pChar, uint8_t* data, const size_t &size);
        void setData(BLECharacteristic* &Char, uint8_t* data, const size_t &size);
        void getData(BLECharacteristic* &pChar, uint8_t &data);
        void setData(BLECharacteristic* &pChar, uint8_t &data);
        char TAG[11];
        bool bInitialized;
        bool bActivated;
        BLEService* pService;
        BLECharacteristic* pNameChar;
        BLECharacteristic* pBatteryLevelChar;
        BLECharacteristic* pCommandChar; 
        BLEUUID service_uuid;
        friend class NameCharCallbacks;
};

class NameCharCallbacks final : public BLECharacteristicCallbacks { //これだけちょっと特殊　BaseServiceのメンバに干渉する
    public:
        NameCharCallbacks(void) = delete;
        NameCharCallbacks(BaseService *_service);
        void onWrite(BLECharacteristic* pNameChar) override;
    private:
        BaseService* service;
        const char* TAG;
};


class BatteryLevelCharCallbacks final : public BLECharacteristicCallbacks {
    public:
        BatteryLevelCharCallbacks(void) = delete;
        BatteryLevelCharCallbacks(BatteryLevelCharCallbackFunc_t batteryLevelCbFunc, const char* _type);
        void onWrite(BLECharacteristic *pBatteryLevelChar) override;
    private:
        BatteryLevelCharCallbackFunc_t batteryLevelCbFunc;
        const char* TAG;
        const char* type;
};


#endif
