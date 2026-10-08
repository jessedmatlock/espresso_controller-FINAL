// eeprom_store.cpp — load/save/init for all EEPROM-persisted parameters.
// Extracted verbatim; no logic changes. Single task only.
#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "error_system.h"
#include "shared_state.h"
#include "state_machine.h"
#include "eeprom_store.h"

// Helper defined in the main .ino.
void recomputeSetupComplete();

// --- Shot/brew parameters (shared with web_api.cpp/state_machine.cpp) ---
extern double originalSetpointTemp;
extern double shotTargetTime;
extern double shotTargetWeight;
extern double preInfusionDuration;
extern double pidBoostDuration;
extern double preInfusionPulseRate;
extern double doseWeight;
extern int cleaningCycleCount;

// --- Scale/fill/pressure calibration values ---
extern float scaleCalibrationFactor;
extern int32_t scaleZeroOffset;
extern int fillProbeThreshold;
extern double pressureOffset;
extern double pressureScale;

// --- Setup-flow flags ---
extern bool setupComplete;
extern bool tempCalComplete;
extern bool pressureCalComplete;
extern bool boilerFilled;
extern bool scaleCalComplete;
extern SystemState currentState;

void writeFloatToEEPROM(int address, float value) {
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    EEPROM.write(address + i, *p++);
  }
}

float readFloatFromEEPROM(int address) {
  float value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(address + i);
  }
  return value;
}

void writeUint16ToEEPROM(int address, uint16_t value) {
  EEPROM.write(address, value & 0xFF);
  EEPROM.write(address + 1, (value >> 8) & 0xFF);
}

uint16_t readUint16FromEEPROM(int address) {
  uint16_t value = EEPROM.read(address);
  value |= (EEPROM.read(address + 1) << 8);
  return value;
}

void saveParametersToEEPROM() {
  // Enhanced protection: Only save when system is in safe state
  if (currentState == STATE_BREW_ACTIVE || currentState == STATE_BREW_START || currentState == STATE_BREW_END || currentState == STATE_FILL) {
    Serial.println("Cannot save parameters during active operations");
    return;
  }

  // Additional safety check: Ensure no critical errors are active
  if (isCriticalError()) {
    Serial.println("Cannot save parameters during critical error state");
    return;
  }

  // Re-entrancy guard (single task, no spinlock needed)
  static volatile bool eepromSaveInProgress = false;
  if (eepromSaveInProgress) {
    Serial.println("EEPROM save already in progress - skipping");
    return;
  }
  eepromSaveInProgress = true;

  // Snapshot mutex-protected values BEFORE any long operations
  // This avoids nested critical sections (S-3) and keeps flash writes outside spinlocks (S-2)
  double snapSetpointTemp = getSetpointTemp();
  bool snapAllowUnsafePid = getAllowUnsafePid();
  double snapKp = getKp();
  double snapKi = getKi();
  double snapKd = getKd();

  // Write signature
  writeUint16ToEEPROM(ADDR_SIGNATURE, EEPROM_SIGNATURE);

  // Write parameters (using snapshots for mutex-protected values)
  writeFloatToEEPROM(ADDR_BREW_TEMP, snapSetpointTemp);
  writeFloatToEEPROM(ADDR_SHOT_TIME, shotTargetTime);
  writeFloatToEEPROM(ADDR_SHOT_WEIGHT, shotTargetWeight);
  writeFloatToEEPROM(ADDR_PREINFUSION, preInfusionDuration);
  writeFloatToEEPROM(ADDR_SCALE_CAL, scaleCalibrationFactor);
  writeFloatToEEPROM(ADDR_PID_BOOST, pidBoostDuration);
  writeFloatToEEPROM(ADDR_PID_KP, snapKp);
  writeFloatToEEPROM(ADDR_PID_KI, snapKi);
  writeFloatToEEPROM(ADDR_PID_KD, snapKd);
  EEPROM.put(ADDR_FILL_THRESHOLD, fillProbeThreshold);
  writeFloatToEEPROM(ADDR_PREINFUSION_RATE, preInfusionPulseRate);
  writeFloatToEEPROM(ADDR_PRESSURE_OFFSET, pressureOffset);
  writeFloatToEEPROM(ADDR_PRESSURE_SCALE, pressureScale);
  writeFloatToEEPROM(ADDR_DOSE_WEIGHT, doseWeight);
  EEPROM.put(ADDR_SCALE_ZERO_OFFSET, scaleZeroOffset);
  EEPROM.put(ADDR_CLEANING_CYCLES, cleaningCycleCount);

  // Write setup state flags (using snapshot for mutex-protected value)
  EEPROM.write(ADDR_SETUP_COMPLETE, setupComplete ? 1 : 0);
  EEPROM.write(ADDR_TEMP_CAL_COMPLETE, tempCalComplete ? 1 : 0);
  EEPROM.write(ADDR_PRESSURE_CAL_COMPLETE, pressureCalComplete ? 1 : 0);
  EEPROM.write(ADDR_BOILER_FILLED, boilerFilled ? 1 : 0);
  EEPROM.write(ADDR_ALLOW_UNSAFE_PID, snapAllowUnsafePid ? 1 : 0);
  EEPROM.write(ADDR_SCALE_CAL_COMPLETE, scaleCalComplete ? 1 : 0);

  // Flash write - safe outside critical section
  EEPROM.commit();

  // Verify write operation
  uint16_t verifySignature = readUint16FromEEPROM(ADDR_SIGNATURE);
  if (verifySignature != EEPROM_SIGNATURE) {
    Serial.println("ERROR: EEPROM write verification failed");
    eepromSaveInProgress = false;
    return;
  }

  eepromSaveInProgress = false;
  Serial.println("Parameters saved to EEPROM (including PID gains and fill threshold) - verified");
}

bool loadParametersFromEEPROM() {
  // Check signature
  uint16_t signature = readUint16FromEEPROM(ADDR_SIGNATURE);
  if (signature != EEPROM_SIGNATURE) {
    Serial.println("EEPROM signature invalid, using defaults");
    return false;
  }

  // Load parameters
  setSetpointTemp(readFloatFromEEPROM(ADDR_BREW_TEMP));
  originalSetpointTemp = getSetpointTemp();  // Initialize original setpoint for compensation
  shotTargetTime = readFloatFromEEPROM(ADDR_SHOT_TIME);
  shotTargetWeight = readFloatFromEEPROM(ADDR_SHOT_WEIGHT);
  preInfusionDuration = readFloatFromEEPROM(ADDR_PREINFUSION);
  scaleCalibrationFactor = readFloatFromEEPROM(ADDR_SCALE_CAL);
  pidBoostDuration = readFloatFromEEPROM(ADDR_PID_BOOST);
  setKp(readFloatFromEEPROM(ADDR_PID_KP));
  setKi(readFloatFromEEPROM(ADDR_PID_KI));
  setKd(readFloatFromEEPROM(ADDR_PID_KD));
  EEPROM.get(ADDR_FILL_THRESHOLD, fillProbeThreshold);
  preInfusionPulseRate = readFloatFromEEPROM(ADDR_PREINFUSION_RATE);
  pressureOffset = readFloatFromEEPROM(ADDR_PRESSURE_OFFSET);
  pressureScale = readFloatFromEEPROM(ADDR_PRESSURE_SCALE);
  doseWeight = readFloatFromEEPROM(ADDR_DOSE_WEIGHT);
  EEPROM.get(ADDR_SCALE_ZERO_OFFSET, scaleZeroOffset);
  EEPROM.get(ADDR_CLEANING_CYCLES, cleaningCycleCount);

  // Load setup state flags
  setupComplete = (EEPROM.read(ADDR_SETUP_COMPLETE) == 1);
  tempCalComplete = (EEPROM.read(ADDR_TEMP_CAL_COMPLETE) == 1);
  pressureCalComplete = (EEPROM.read(ADDR_PRESSURE_CAL_COMPLETE) == 1);
  boilerFilled = (EEPROM.read(ADDR_BOILER_FILLED) == 1);
  setAllowUnsafePid(EEPROM.read(ADDR_ALLOW_UNSAFE_PID) == 1);
  scaleCalComplete = (EEPROM.read(ADDR_SCALE_CAL_COMPLETE) == 1);

  // Update setupComplete based on required calibrations
  recomputeSetupComplete();

  // Validate parameter ranges
  if (getSetpointTemp() < 180.0 || getSetpointTemp() > 220.0) {
    setSetpointTemp(200.0);
    originalSetpointTemp = 200.0;
  }
  if (shotTargetTime < 0 || shotTargetTime > 60.0) shotTargetTime = 30.0;
  if (shotTargetWeight < 0 || shotTargetWeight > 60.0) shotTargetWeight = 36.0;
  if (preInfusionDuration < 0 || preInfusionDuration > shotTargetTime) preInfusionDuration = 8.0;
  if (pidBoostDuration < 0 || pidBoostDuration > shotTargetTime) pidBoostDuration = 4.0;
  if (scaleCalibrationFactor < 0.1 || scaleCalibrationFactor > 10.0) scaleCalibrationFactor = 1.0;
  if (getKp() < 0.1 || getKp() > 200.0) setKp(40.0);
  if (getKi() < 0.0 || getKi() > 10.0) setKi(0.8);
  if (getKd() < 0.0 || getKd() > 500.0) setKd(120.0);
  if (fillProbeThreshold < 100 || fillProbeThreshold > 900) fillProbeThreshold = 512;
  if (preInfusionPulseRate < 100 || preInfusionPulseRate > 1000) preInfusionPulseRate = 500;  // 100-1000ms range, user configurable
  if (pressureOffset < -1.0 || pressureOffset > 4.0) pressureOffset = 0.0;  // Voltage units (0-3.3V range)
  if (pressureScale < 0.1 || pressureScale > 100.0) pressureScale = 1.0;
  if (doseWeight < 10.0 || doseWeight > 30.0) doseWeight = 18.0;
  if (cleaningCycleCount < 1 || cleaningCycleCount > 20) cleaningCycleCount = 10;

  Serial.println("Parameters loaded from EEPROM");
  Serial.printf("Brew Temp: %.1f°F, Shot Time: %.1fs, Shot Weight: %.1fg\n",
                getSetpointTemp(), shotTargetTime, shotTargetWeight);
  Serial.printf("Pre-infusion: %.1fs, PID Boost: %.1fs, Scale Cal: %.2f\n",
                preInfusionDuration, pidBoostDuration, scaleCalibrationFactor);
  Serial.printf("PID Gains: Kp=%.1f, Ki=%.2f, Kd=%.1f, Fill Threshold=%d\n",
                getKp(), getKi(), getKd(), (int)fillProbeThreshold);

  return true;
}

void initEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  if (!loadParametersFromEEPROM()) {
    // First time setup - save defaults
    Serial.println("First time setup - saving default parameters");
    saveParametersToEEPROM();
  }
}
