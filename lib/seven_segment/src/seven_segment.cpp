#include "seven_segment.hpp"

const uint8_t SevenSegment::num_pattern[10] = { 0b11111100, 0b01100000, 0b11011010, 0b11110010, 0b01100110, 0b10110110, 0b10111110, 0b11100100, 0b11111110, 0b11110110 };
const char* SevenSegment::TAG = "7 Segment";
SevenSegment::SevenSegment(uint8_t APin, uint8_t BPin, uint8_t CPin, uint8_t DPin, uint8_t EPin, uint8_t FPin, uint8_t GPin, uint8_t DPPin)
  : _APin(APin),
    _BPin(BPin),
    _CPin(CPin),
    _DPin(DPin),
    _EPin(EPin),
    _FPin(FPin),
    _GPin(GPin),
    _DPPin(DPPin){
    pinMode(_APin, OUTPUT);
    pinMode(_BPin, OUTPUT);
    pinMode(_CPin, OUTPUT);
    pinMode(_DPin, OUTPUT);
    pinMode(_EPin, OUTPUT);
    pinMode(_FPin, OUTPUT);
    pinMode(_GPin, OUTPUT);
    pinMode(_DPPin, OUTPUT);
}

SevenSegment::~SevenSegment(void) {
  pinMode(_APin, INPUT);
  pinMode(_BPin, INPUT);
  pinMode(_CPin, INPUT);
  pinMode(_DPin, INPUT);
  pinMode(_EPin, INPUT);
  pinMode(_FPin, INPUT);
  pinMode(_GPin, INPUT);
  pinMode(_DPPin, INPUT);
}


uint8_t SevenSegment::num_to_pattern(const int num) {
  if (num < 0 || num >= 10) {
    return 0;
  }
  return num_pattern[num];
}

void SevenSegment::setPattern(uint8_t APin_Val, uint8_t BPin_Val, uint8_t CPin_Val, uint8_t DPin_Val, uint8_t EPin_Val, uint8_t FPin_Val, uint8_t GPin_Val, uint8_t DPPin_Val) {
  digitalWrite(_APin, APin_Val);
  digitalWrite(_BPin, BPin_Val);
  digitalWrite(_CPin, CPin_Val);
  digitalWrite(_DPin, DPin_Val);
  digitalWrite(_EPin, EPin_Val);
  digitalWrite(_FPin, FPin_Val);
  digitalWrite(_GPin, GPin_Val);
  digitalWrite(_DPPin, DPPin_Val);
}


void SevenSegment::Number(const uint8_t num, const bool dot) {
    uint8_t pattern = num_to_pattern(num);
    digitalWrite(_APin, bitRead(pattern, 7));
    digitalWrite(_BPin, bitRead(pattern, 6));
    digitalWrite(_CPin, bitRead(pattern, 5));
    digitalWrite(_DPin, bitRead(pattern, 4));
    digitalWrite(_EPin, bitRead(pattern, 3));
    digitalWrite(_FPin, bitRead(pattern, 2));
    digitalWrite(_GPin, bitRead(pattern, 1));
    digitalWrite(_DPPin, dot);
    ESP_LOGD(TAG, "number:%d dot:%d", num, dot);
}

void SevenSegment::Off(void) {
    setPattern(LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW);
}