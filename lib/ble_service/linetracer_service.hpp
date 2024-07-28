#ifndef LINETRACER
#define LINETRACER

#include "base_service.hpp"

extern BLEUUID linedata_char_uuid;
extern BLEUUID linetracer_service_uuid;

struct LineData {
    public:
        byte num_of_reflector;
        byte bValue[6];
        short iValue[6];
};



class LineTracerService final : public BaseService {
    public:
        static LineTracerService* getInstance(void);
        void init(BLEServer* pServer) override;
        bool getLineData(LineData& lineData);

    private:
        LineTracerService(void);
        ~LineTracerService(void);
        static LineTracerService* lineTracer;
        BLECharacteristic *pLineDataChar;
};

#endif