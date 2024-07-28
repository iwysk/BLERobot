#ifndef LINETRACER_SERVICE_MANAGER
#define LINETRACER_SERVICE_MANAGER

#include "service_manager.hpp"

extern const BLEUUID linetracer_service_uuid;
extern const BLEUUID line_data_char_uuid;

typedef struct {
    public:
        byte num_of_reflector;
        byte bValue;
        short iValue[8];
        bool isInterSection;
        uint8_t Intersection_Angle[6];
} LineData;

class LineTracerServiceManager final : public ServiceManager {
    public:
        static LineTracerServiceManager* getInstance(void);
        bool init(BLEClient* _pClient);
        bool getLineData(LineData &lineData);

    private:
        LineTracerServiceManager(void);
        ~LineTracerServiceManager(void);
        static LineTracerServiceManager *lineTracer;
        BLERemoteService *pLineTracerService;
        BLERemoteCharacteristic *pLineDataChar;
};


#endif