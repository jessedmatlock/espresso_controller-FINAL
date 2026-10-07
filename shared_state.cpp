// shared_state.cpp — storage + mutex-protected accessors for every value that
// crosses the Core 0 / Core 1 boundary. See shared_state.h for the contract.
#include "shared_state.h"

static ControlInputs g_controlInputs;
static ControlOutputs g_controlOutputs;
static double g_cachedRawTemp = 77.0;

// inputsMux guards every field of g_controlInputs.
// outputsMux guards every field of g_controlOutputs.
// cachedTempMux guards g_cachedRawTemp only (different cadence/producer than
// the rest of ControlInputs, kept as its own lock as in the original design).
static portMUX_TYPE inputsMux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE outputsMux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE cachedTempMux = portMUX_INITIALIZER_UNLOCKED;

// --- ControlInputs ---
double getSetpointTemp() {
  portENTER_CRITICAL(&inputsMux);
  double temp = g_controlInputs.setpointTemp;
  portEXIT_CRITICAL(&inputsMux);
  return temp;
}

void setSetpointTemp(double temp) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.setpointTemp = temp;
  portEXIT_CRITICAL(&inputsMux);
}

void addSetpointTemp(double delta) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.setpointTemp += delta;
  portEXIT_CRITICAL(&inputsMux);
}

double getKp() {
  portENTER_CRITICAL(&inputsMux);
  double value = g_controlInputs.Kp;
  portEXIT_CRITICAL(&inputsMux);
  return value;
}

void setKp(double value) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.Kp = value;
  portEXIT_CRITICAL(&inputsMux);
}

double getKi() {
  portENTER_CRITICAL(&inputsMux);
  double value = g_controlInputs.Ki;
  portEXIT_CRITICAL(&inputsMux);
  return value;
}

void setKi(double value) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.Ki = value;
  portEXIT_CRITICAL(&inputsMux);
}

double getKd() {
  portENTER_CRITICAL(&inputsMux);
  double value = g_controlInputs.Kd;
  portEXIT_CRITICAL(&inputsMux);
  return value;
}

void setKd(double value) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.Kd = value;
  portEXIT_CRITICAL(&inputsMux);
}

bool getPidBoostActive() {
  portENTER_CRITICAL(&inputsMux);
  bool active = g_controlInputs.pidBoostActive;
  portEXIT_CRITICAL(&inputsMux);
  return active;
}

void setPidBoostActive(bool active) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.pidBoostActive = active;
  portEXIT_CRITICAL(&inputsMux);
}

bool getPreInfusionPhase() {
  portENTER_CRITICAL(&inputsMux);
  bool phase = g_controlInputs.preInfusionPhase;
  portEXIT_CRITICAL(&inputsMux);
  return phase;
}

void setPreInfusionPhase(bool phase) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.preInfusionPhase = phase;
  portEXIT_CRITICAL(&inputsMux);
}

bool getPreInfusionActive() {
  portENTER_CRITICAL(&inputsMux);
  bool active = g_controlInputs.preInfusionActive;
  portEXIT_CRITICAL(&inputsMux);
  return active;
}

void setPreInfusionActive(bool active) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.preInfusionActive = active;
  portEXIT_CRITICAL(&inputsMux);
}

bool getAllowUnsafePid() {
  portENTER_CRITICAL(&inputsMux);
  bool allowed = g_controlInputs.allowUnsafePid;
  portEXIT_CRITICAL(&inputsMux);
  return allowed;
}

void setAllowUnsafePid(bool allowed) {
  portENTER_CRITICAL(&inputsMux);
  g_controlInputs.allowUnsafePid = allowed;
  portEXIT_CRITICAL(&inputsMux);
}

// --- ControlOutputs ---
double getTemp() {
  portENTER_CRITICAL(&outputsMux);
  double temp = g_controlOutputs.filteredTemp;
  portEXIT_CRITICAL(&outputsMux);
  return temp;
}

void setTemp(double temp) {
  portENTER_CRITICAL(&outputsMux);
  g_controlOutputs.filteredTemp = temp;
  portEXIT_CRITICAL(&outputsMux);
}

double getCurrentPIDOutputPercent() {
  portENTER_CRITICAL(&outputsMux);
  double percent = g_controlOutputs.pidOutputPercent;
  portEXIT_CRITICAL(&outputsMux);
  return percent;
}

void setCurrentPIDOutputPercent(double percent) {
  portENTER_CRITICAL(&outputsMux);
  g_controlOutputs.pidOutputPercent = percent;
  portEXIT_CRITICAL(&outputsMux);
}

bool getBoilerHeatingState() {
  portENTER_CRITICAL(&outputsMux);
  bool on = g_controlOutputs.boilerHeating;
  portEXIT_CRITICAL(&outputsMux);
  return on;
}

void setBoilerHeatingState(bool on) {
  portENTER_CRITICAL(&outputsMux);
  g_controlOutputs.boilerHeating = on;
  portEXIT_CRITICAL(&outputsMux);
}

// --- Cached raw temperature ---
double getCachedRawTemp() {
  portENTER_CRITICAL(&cachedTempMux);
  double temp = g_cachedRawTemp;
  portEXIT_CRITICAL(&cachedTempMux);
  return temp;
}

void setCachedRawTemp(double temp) {
  portENTER_CRITICAL(&cachedTempMux);
  g_cachedRawTemp = temp;
  portEXIT_CRITICAL(&cachedTempMux);
}
