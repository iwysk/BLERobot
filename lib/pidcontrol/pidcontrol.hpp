#ifndef PID_CONTROL_H
#define PID_CONTROL_H

#include <Arduino.h>

struct PIDParameters {
    double pGain, iGain, dGain;
};

class PIDControl final {
    public:
        PIDControl(const PIDParameters _parameters);
        void setParameters(const PIDParameters _parameters);
        void reset(void);
        double compute(const double error);
    private:
        PIDParameters parameters;
        double iError;
        double error_old;
        bool isFirst;
};


#endif