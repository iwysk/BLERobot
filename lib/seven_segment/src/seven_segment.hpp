#ifndef SEVENSEGMENT_H
#define SEVENSEGMENT_H
#include <Arduino.h>

class SevenSegment final {
public:
    SevenSegment() = delete;
    SevenSegment(uint8_t APin, uint8_t BPin, uint8_t CPin, uint8_t DPin, uint8_t EPin, uint8_t FPin, uint8_t GPin);
    ~SevenSegment(void);
    uint8_t num_to_pattern(const int num);
    void setPattern(uint8_t APin_Val, uint8_t BPin_Val, uint8_t CPin_Val, uint8_t DPin_Val, uint8_t EPin_Val, uint8_t FPin_Val, uint8_t GPin_Val);
    void Number(const uint8_t num);
    void Off(void);
private:
    static const char* TAG;
    static const uint8_t num_pattern[10];
    uint8_t _APin, _BPin, _CPin, _DPin, _EPin, _FPin, _GPin;
};

#endif