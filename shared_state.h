// shared_state.h — the ONLY place cross-core (Core 0 <-> Core 1 / ControlTask)
// data is allowed to live. Every field here is reachable from both
// systemTask (Core 0) and controlTask (Core 1); every access goes through one
// of the functions below, never through a bare global.
//
// ControlInputs: written by Core 0 (web handlers, serial commands, EEPROM
//   load, brew routine), read by Core 1 (pid_step()).
// ControlOutputs: written by Core 1 (pid_step()), read by Core 0 (display,
//   JSON status, web API).
// cachedRawTemp: written by Core 0 (handlePIDUpdates(), ~50ms SPI read cache),
//   read by Core 1 (pid_step()) — unchanged from the original design, just
//   colocated here since it's also a cross-core primitive.
//
// Nothing outside this .h/.cpp pair should declare a portMUX_TYPE for
// PID-related state. If a new PID-adjacent value needs to cross cores, it
// belongs in one of the two structs below, with its own getter/setter added
// to this file — that is the enforcement mechanism for keeping this boundary
// closed.
#pragma once
#include <Arduino.h>

struct ControlInputs {
  double setpointTemp = 200.0;
  double Kp = 40.0;
  double Ki = 0.8;
  double Kd = 120.0;
  bool pidBoostActive = false;
  bool preInfusionPhase = false;
  bool preInfusionActive = false;
  bool allowUnsafePid = false;
};

struct ControlOutputs {
  double filteredTemp = 77.0;       // replaces the old bare `currentTemp`
  double pidOutputPercent = 0.0;    // replaces the old bare `currentPIDOutputPercent`
  bool boilerHeating = false;       // was unprotected before this refactor
};

// --- ControlInputs accessors (Core 0 writes, Core 1 reads) ---
double getSetpointTemp();
void setSetpointTemp(double temp);
void addSetpointTemp(double delta);

double getKp();
void setKp(double value);
double getKi();
void setKi(double value);
double getKd();
void setKd(double value);

bool getPidBoostActive();
void setPidBoostActive(bool active);
bool getPreInfusionPhase();
void setPreInfusionPhase(bool phase);
bool getPreInfusionActive();
void setPreInfusionActive(bool active);

bool getAllowUnsafePid();
void setAllowUnsafePid(bool allowed);

// --- ControlOutputs accessors (Core 1 writes, Core 0 reads) ---
double getTemp();
void setTemp(double temp);

double getCurrentPIDOutputPercent();
void setCurrentPIDOutputPercent(double percent);

bool getBoilerHeatingState();
void setBoilerHeatingState(bool on);

// --- Cached raw temperature (Core 0 writes @ ~50ms, Core 1 reads) ---
// Unchanged from the original design — relocated here only for colocation.
double getCachedRawTemp();
void setCachedRawTemp(double temp);
