#ifndef UNIT_MANAGER 
#define UNIT_MANAGER
#include <BLEDevice.h>

typedef struct {
    public:
        uint8_t command;
} Command;

class UnitManager { //ユニット制御用基底クラス オーバーライド前提
    public:             
        virtual bool init(BLEClient* pClient) = 0;
        bool getInitialized(void) const;
        bool getBatteryLevel(uint8_t &battery_level);
        bool sendCommand(BLERemoteCharacteristic* &pChar, const Command& command);

    protected:
        UnitManager(BLEUUID _service_uuid);
        ~UnitManager(void);
        bool InitService(BLEClient* pClient); 
        bool getData(BLERemoteCharacteristic* &pChar, uint8_t* data, const size_t &size);
        bool setData(BLERemoteCharacteristic* &Char, uint8_t* data, const size_t &size);
        bool getData(BLERemoteCharacteristic* &pChar, uint8_t &data);
        bool setData(BLERemoteCharacteristic* &pChar, const uint8_t &data);
        char TAG[15];
        BLERemoteService *pService;
        BLERemoteCharacteristic *pBatteryLevelChar;
        BLEUUID service_uuid;
        static const BLEUUID battery_level_char_uuid;
        bool bInitialized;
};


#endif