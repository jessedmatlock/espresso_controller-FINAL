// shared_state.cpp — storage + accessors for PID parameters/results. See
// shared_state.h for why these are plain reads/writes now (single task, no
// concurrent access possible) rather than mutex-protected.
#include "shared_state.h"

static ControlInputs g_controlInputs;
static ControlOutputs g_controlOutputs;
static double g_cachedRawTemp = 77.0;

// --- ControlInputs ---
double getSetpointTemp() { return g_controlInputs.setpointTemp; }
void setSetpointTemp(double temp) { g_controlInputs.setpointTemp = temp; }
void addSetpointTemp(double delta) { g_controlInputs.setpointTemp += delta; }

double getKp() { return g_controlInputs.Kp; }
void setKp(double value) { g_controlInputs.Kp = value; }
double getKi() { return g_controlInputs.Ki; }
void setKi(double value) { g_controlInputs.Ki = value; }
double getKd() { return g_controlInputs.Kd; }
void setKd(double value) { g_controlInputs.Kd = value; }

bool getPidBoostActive() { return g_controlInputs.pidBoostActive; }
void setPidBoostActive(bool active) { g_controlInputs.pidBoostActive = active; }
bool getPreInfusionPhase() { return g_controlInputs.preInfusionPhase; }
void setPreInfusionPhase(bool phase) { g_controlInputs.preInfusionPhase = phase; }
bool getPreInfusionActive() { return g_controlInputs.preInfusionActive; }
void setPreInfusionActive(bool active) { g_controlInputs.preInfusionActive = active; }

bool getAllowUnsafePid() { return g_controlInputs.allowUnsafePid; }
void setAllowUnsafePid(bool allowed) { g_controlInputs.allowUnsafePid = allowed; }

// --- ControlOutputs ---
double getTemp() { return g_controlOutputs.filteredTemp; }
void setTemp(double temp) { g_controlOutputs.filteredTemp = temp; }

double getCurrentPIDOutputPercent() { return g_controlOutputs.pidOutputPercent; }
void setCurrentPIDOutputPercent(double percent) { g_controlOutputs.pidOutputPercent = percent; }

bool getBoilerHeatingState() { return g_controlOutputs.boilerHeating; }
void setBoilerHeatingState(bool on) { g_controlOutputs.boilerHeating = on; }

// --- Cached raw temperature ---
double getCachedRawTemp() { return g_cachedRawTemp; }
void setCachedRawTemp(double temp) { g_cachedRawTemp = temp; }
