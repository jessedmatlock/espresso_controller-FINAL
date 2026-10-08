// shared_state.h — central storage for PID-related parameters and results
// (setpoint, gains, boost/pre-infusion flags, filtered temp, PID output
// percent, boiler-heating state, cached raw temp). Everything here is
// reachable from multiple files (pid_control.cpp, web_api.cpp,
// eeprom_store.cpp, calibration.cpp, display.cpp) which is why it's
// centralized behind named accessors rather than scattered bare globals —
// that's an encapsulation choice, not a synchronization one. This firmware
// runs as a single FreeRTOS task, so there is no concurrent access to guard
// against; an earlier revision had these accessors take a mutex each,
// because Kp/Ki/Kd and boilerHeating used to cross a Core 0/Core 1 task
// boundary. That boundary no longer exists (see pid_control.h).
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
  bool boilerHeating = false;
};

// --- ControlInputs accessors ---
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

// --- ControlOutputs accessors ---
double getTemp();
void setTemp(double temp);

double getCurrentPIDOutputPercent();
void setCurrentPIDOutputPercent(double percent);

bool getBoilerHeatingState();
void setBoilerHeatingState(bool on);

// --- Cached raw temperature ---
// Refreshed every ~50ms by handlePIDUpdates(), consumed every ~100ms by
// updatePidControl(). Kept as its own named pair (not folded into
// ControlInputs) because it has its own producer/cadence — see the comment
// on handlePIDUpdates() for why this indirection is kept even though PID
// could now call the RTD read directly.
double getCachedRawTemp();
void setCachedRawTemp(double temp);
