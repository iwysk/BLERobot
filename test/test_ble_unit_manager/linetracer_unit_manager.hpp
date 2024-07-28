#ifndef LINETRACER_UNIT_MANAGER
#define LINETRACER_UNIT_MANAGER

#include <Arduino.h>
#include <BLEDevice.h>
#include "unit_manager.hpp"

typedef struct {
    public:
        byte num_of_reflector;
        byte bValue;
        short iValue[8];
        bool isInterSection;
        uint8_t Intersection_Angle[6];
} LineData;

class LineTracerUnitManager final : public UnitManager {
    public:
        static LineTracerUnitManager* getInstance(BLEClient* pClient, const bool& bInitialize, bool& bInitialized);
        bool init(BLEClient* pClient);
        bool getLineData(LineData &lineData);

    private:
        LineTracerUnitManager(void);
        ~LineTracerUnitManager(void);
        static LineTracerUnitManager *lineTracerUnit;
        BLERemoteService *pLineTracerService;
        BLERemoteCharacteristic *pLineDataChar;
        static const BLEUUID linetracer_service_uuid;
        static const BLEUUID line_data_char_uuid;
};


#endif