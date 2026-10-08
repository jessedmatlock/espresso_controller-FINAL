// sensors.h — all sensor bus I/O (SPI/I2C/ADC). pid_control.cpp never calls
// into this file directly; it only ever reads the cached/filtered values
// these functions publish via shared_state.h.
#pragma once

// RTD (MAX31865 PT100, SPI)
bool initRTDSensor();
double read_boiler_temp();
double celsiusToFahrenheit(double celsius);
double fahrenheitToCelsius(double fahrenheit);

// Scale (NAU7802, I2C)
bool initScale();
void readScale();
void tareScale();
void calculateFlowRate();
void calculateShotRatio();

// Fill probe (analog)
bool readFillProbe();
int readFillProbeRaw();
void initFillSystem();

// Pressure transducer (analog)
void initPressure();
double readPressure();
