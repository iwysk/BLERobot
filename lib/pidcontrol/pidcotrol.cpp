#include <pidcontrol.hpp>

PIDControl::PIDControl(const PIDParameters _parameters) : parameters(_parameters), isFirst(true), iError(0) {
}


void PIDControl::setParameters(const PIDParameters _parameters) {
    parameters = _parameters;
    isFirst = true;
    iError = 0;
}

void PIDControl::reset(void) {
    isFirst = true;
    iError = 0;
}

double PIDControl::compute(const double error) {
    if (isFirst) {
        isFirst = false;
        error_old = error;
    }
    double pError = error;
    iError += error;
    double dError = error - error_old;
    error_old = error;
    return parameters.pGain * pError + parameters.iGain * iError + parameters.dGain * dError;
}