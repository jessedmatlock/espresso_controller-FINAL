// calibration.h — fill/scale/pressure calibration state machine. Core 0
// only; shared broadly (state machine, web handlers) since isPressureCal()/
// isFillScaleCal() gate brew/fill behavior outside calibration.cpp itself.
#pragma once
#include <Arduino.h>

enum CalStep : uint8_t {
  CAL_IDLE,
  CAL_FILL_DRY, CAL_FILL_WET,
  CAL_SCALE_ZERO, CAL_SCALE_WEIGHT, CAL_SCALE_CONFIRM,
  CAL_PRESSURE_ZERO, CAL_PRESSURE_HIGH
};

extern CalStep calStep;

inline bool isCalibrating() { return calStep != CAL_IDLE; }
inline bool isPressureCal() { return calStep == CAL_PRESSURE_ZERO || calStep == CAL_PRESSURE_HIGH; }
inline bool isFillScaleCal() { return calStep >= CAL_FILL_DRY && calStep <= CAL_SCALE_CONFIRM; }

const char* calStepName(CalStep s);

void startFillProbeCalibration();
void startScaleCalibration();
void processCalibrationStep();
void completeScaleCalibration(float knownWeight);

void startPressureCalibration();
void processPressureCalibrationStep();
void completePressureCalibration();
