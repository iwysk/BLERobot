#ifndef UNIT_MANAGER 
#define UNIT_MANAGER
#include <Arduino.h>
#include <BLEDevice.h>

extern const BLEUUID name_char_uuid;
extern const BLEUUID battery_level_char_uuid;
extern const BLEUUID command_char_uuid;

struct Command {
    public:
        uint8_t command;
        int parameter;
};

typedef void (*CommandCharCallbackFunc_t)(BLERemoteCharacteristic* pCommandChar, uint8_t* data, size_t length, bool isNotify);

class ServiceManager { //基底クラス
    public:
        virtual bool init(BLEClient* _pClient) = 0;
        bool getInitialized(void) const;
        bool setName(std::string Name);
        bool setBatteryLevel(const uint8_t &battery_level);
        bool getCommand(Command& command);
        bool setCommand(const Command& command);
        bool setCommandCallback(CommandCharCallbackFunc_t _CommandCbFunc);

    protected:
        ServiceManager(BLEUUID _service_uuid);
        ~ServiceManager(void);
        bool InitService(void);
        bool getData(BLERemoteCharacteristic* &pChar, uint8_t* data, const size_t &size);
        bool setData(BLERemoteCharacteristic* &Char, uint8_t* data, const size_t &size);
        bool getData(BLERemoteCharacteristic* &pChar, uint8_t &data);
        bool setData(BLERemoteCharacteristic* &pChar, const uint8_t &data);
        
        char TAG[15];
        BLEClient* pClient;
        BLERemoteService *pService;
        BLERemoteCharacteristic *pNameChar;
        BLERemoteCharacteristic *pBatteryLevelChar;
        BLERemoteCharacteristic *pCommandChar;
        BLEUUID service_uuid;
        bool bInitialized;
};

#endif