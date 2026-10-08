// sensors.cpp — RTD, scale, fill probe, and pressure transducer reads.
// Extracted verbatim from the original functions; no logic changes.
// calibrateScale(float) was removed — it had zero callers (dead code, same
// class of issue as the removed set_ssr()).
#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include <Adafruit_NAU7802.h>
#include "config.h"
#include "error_system.h"
#include "shared_state.h"
#include "state_machine.h"
#include "relays.h"
#include "sensors.h"

// Hardware objects — defined in the main .ino (setup() constructs them with
// the pin config from config.h before any sensor init runs).
extern Adafruit_MAX31865 rtdSensor;
extern Adafruit_NAU7802 nau;

// Helpers defined in the main .ino.
bool millisElapsed(unsigned long startTime, unsigned long interval);
void recomputeSetupComplete();

// --- Setup/calibration flow flags (owned by the main .ino, shared broadly) ---
extern bool tempCalComplete;
extern bool setupComplete;

// --- Scale state (shared with calibration.cpp/eeprom_store.cpp/web_api.cpp) ---
extern bool scaleConnected;
extern bool scaleInitialized;
extern float scaleCalibrationFactor;
extern int32_t scaleZeroOffset;
extern double shotWeight;
extern bool brewActive;
extern bool scaleReliableEntireShot;
extern double doseWeight;
extern double currentFlowRate;
extern double lastWeightForFlow;
extern unsigned long lastFlowCalculation;
extern double smoothedFlowRate;
extern bool flowRateFirstCalculation;
extern double currentShotRatio;

// --- Fill probe state (shared with state_machine) ---
extern bool fillProbeWet;
extern int fillProbeThreshold;

// --- Pressure state (shared with calibration.cpp/eeprom_store.cpp/web_api.cpp) ---
extern double currentPressure;
extern double pressureOffset;
extern double pressureScale;
extern bool pressureLockoutActive;
extern SystemState currentState;

// --- RTD-internal state: only read_boiler_temp()/initRTDSensor() touch these ---
static double lastTemp = 77.0;
static unsigned long lastTempTime = 0;

// --- Pressure-internal state: only readPressure() touches these ---
static double filteredPressure = 0.0;
static bool pressureFirstReading = true;
static const double pressureAlpha = 0.2;  // Smoothing factor (0.2 = light smoothing)

// --- Temperature Conversion Functions ---
double celsiusToFahrenheit(double celsius) {
  return (celsius * 9.0 / 5.0) + 32.0;
}

double fahrenheitToCelsius(double fahrenheit) {
  return (fahrenheit - 32.0) * 5.0 / 9.0;
}

// --- MAX31865 PT100 RTD Temperature Reading ---
bool initRTDSensor() {
  if (!rtdSensor.begin(MAX31865_3WIRE)) {
    Serial.println("MAX31865 RTD sensor initialization failed");
    setError(ERR_FLAG_RTD);
    return false;
  }

  Serial.println("MAX31865 RTD sensor initialized (3-wire PT100)");

  // Auto-confirm temperature probe on successful initialization
  // PT100 is a precision sensor - if hardware detects no faults and reading is valid,
  // the probe is working correctly. Small offsets don't affect safety (25°F margin to cutoff).
  // Take initial reading to validate probe
  delay(50);  // Brief delay for first reading stability
  float initialTemp = rtdSensor.temperature(RNOMINAL, RREF);
  float initialTempF = (initialTemp * 9.0 / 5.0) + 32.0;

  // Check for faults and validate reading is in reasonable range (32-250°F)
  uint8_t fault = rtdSensor.readFault();
  if (fault == 0 && initialTempF > 32.0 && initialTempF < 250.0) {
    tempCalComplete = true;
    recomputeSetupComplete();
    Serial.printf("Temperature probe auto-confirmed: %.1f°F\n", initialTempF);
    if (setupComplete) {
      Serial.println("All required calibrations complete - PID enabled");
    }
  } else {
    if (fault != 0) {
      Serial.printf("Temperature probe fault detected: 0x%02X - manual verification required\n", fault);
      rtdSensor.clearFault();
    } else {
      Serial.printf("Temperature reading out of range: %.1f°F - manual verification required\n", initialTempF);
    }
  }

  return true;
}

double read_boiler_temp() {
  static unsigned long lastRead = 0;
  static double lastValidTemp = 77.0;  // Start at room temperature

  unsigned long now = millis();
  if (now - lastRead < 250) {  // Read every 250ms to avoid overwhelming the sensor
    return getTemp();
  }

  lastRead = now;

  // Read RTD resistance
  uint16_t rtdValue = rtdSensor.readRTD();

  // Check for RTD faults
  uint8_t fault = rtdSensor.readFault();
  if (fault) {
    Serial.printf("RTD Fault detected: 0x%02X\n", fault);

    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold Fault");
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold Fault");
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("RTD REFIN- > 0.85 x Bias");
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("RTD REFIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTD RTDIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("RTD Under/Over voltage fault");
    }

    rtdSensor.clearFault();  // Clear the fault register
    setError(ERR_FLAG_RTD);

    // Return last known good temperature
    setTemp(lastValidTemp);
    return lastValidTemp;
  }

  // Calculate temperature from resistance
  float resistance = rtdValue;
  resistance /= 32768;
  resistance *= RREF;

  // Convert resistance to temperature (Celsius) using PT100 formula
  double tempCelsius = rtdSensor.temperature(RNOMINAL, RREF);

  // Validate temperature reading
  if (tempCelsius < -50.0 || tempCelsius > 200.0) {
    Serial.printf("Invalid temperature reading: %.2f°C\n", tempCelsius);
    setError(ERR_FLAG_RTD);
    setTemp(lastValidTemp);
    return lastValidTemp;
  }

  // Convert to Fahrenheit
  double tempFahrenheit = celsiusToFahrenheit(tempCelsius);

  // RTD faults no longer auto-clear here. A sensor fault means the last
  // several readings weren't trustworthy, and auto-resuming control the
  // instant one good reading comes back is exactly how a loose/intermittent
  // RTD connection would cause heating to flicker on and off automatically —
  // undermining the "manual reboot required" intent requiresManualReboot()
  // already applies to this flag. Clearing ERR_FLAG_RTD now requires the
  // explicit /calibrate/temp/complete action (handleCalibrateTempComplete()),
  // the same path tempCalComplete already goes through, instead of happening
  // as a silent side effect of this read.

  // Temperature rate-of-change safety check
  if (lastTempTime > 0) {
    double timeDiff = (now - lastTempTime) / 1000.0;  // seconds
    if (timeDiff > 0) {
      double tempRate = abs(tempFahrenheit - lastTemp) / timeDiff;
      if (tempRate > MAX_TEMP_RATE) {
        Serial.printf("TEMPERATURE RATE ERROR: %.1f°F/s (max: %.1f°F/s)\n", tempRate, MAX_TEMP_RATE);
        setError(ERR_FLAG_TEMP);
        return lastValidTemp;  // Return last known good temperature
      }
    }
  }
  lastTemp = tempFahrenheit;
  lastTempTime = now;

  // Update current temperature
  lastValidTemp = tempFahrenheit;
  setTemp(tempFahrenheit);

  return tempFahrenheit;
}

// --- NAU7802 Scale Functions ---
bool initScale() {
  if (!nau.begin()) {
    scaleInitialized = false;
    scaleConnected = false;
    Serial.println("NAU7802 scale initialization failed");
    return false;
  }

  // Configure NAU7802
  nau.setLDO(NAU7802_3V3);                 // 3.3V load cell excitation
  nau.setGain(NAU7802_GAIN_128);           // Maximum sensitivity
  nau.setRate(NAU7802_RATE_80SPS);         // 80 samples per second
  nau.calibrate(NAU7802_CALMOD_INTERNAL);  // Internal ADC offset calibration

  scaleInitialized = true;
  scaleConnected = true;
  Serial.println("NAU7802 scale initialized (3V3, GAIN_128, 80SPS)");
  return true;
}

void readScale() {
  // Scale reading stability validation: these statics are local to this
  // function on purpose (only readScale() needs them across calls).
  static float lastValidScaleWeight = 0;
  static int scaleStableReadings = 0;
  static bool scaleStabilityReset = false;

  if (!scaleInitialized) {
    scaleConnected = false;
    // If scale fails during brew, mark as unreliable - will stop by time instead
    if (brewActive && scaleReliableEntireShot) {
      scaleReliableEntireShot = false;
      Serial.println("Scale became unreliable during brew - switching to time-based stop");
    }
    setError(ERR_FLAG_SCALE);
    return;
  }

  if (nau.available()) {
    int32_t rawReading = nau.read();
    float reading = (rawReading - scaleZeroOffset) / scaleCalibrationFactor;
    if (reading >= -1000 && reading <= 1000) {  // Sanity check

      // Scale reading stability validation - optimized for brew progression
      const int requiredStableReadings = 2;     // Reduced from 3 to 2 for responsiveness
      const float maxWeightDrift = 1.0;         // Increased from 0.2g to 1.0g for brew progression

      // Reset stability variables when brew starts
      if (brewActive && !scaleStabilityReset) {
        lastValidScaleWeight = 0;
        scaleStableReadings = 0;
        scaleStabilityReset = true;
      } else if (!brewActive) {
        scaleStabilityReset = false;
      }

      // During brew, allow weight increases (normal progression)
      if (brewActive && reading > lastValidScaleWeight) {
        shotWeight = reading;  // Accept increasing weight during brew
        lastValidScaleWeight = reading;
        scaleStableReadings = 0;  // Reset stability counter
        scaleConnected = true;
        clearError(ERR_FLAG_SCALE);
      } else if (abs(reading - lastValidScaleWeight) < maxWeightDrift) {
        // Stability check for non-brew or decreasing weight
        scaleStableReadings++;
        if (scaleStableReadings >= requiredStableReadings) {
          shotWeight = reading;  // Accept stable reading
          scaleConnected = true;
          clearError(ERR_FLAG_SCALE);
        }
      } else {
        scaleStableReadings = 0;
        lastValidScaleWeight = reading;
        // Keep previous shotWeight until stable reading achieved
        scaleConnected = true;  // Scale is responding, just not stable
      }
    } else {
      scaleConnected = false;
      // If scale gives bad readings during brew, mark as unreliable
      if (brewActive && scaleReliableEntireShot) {
        scaleReliableEntireShot = false;
        Serial.println("Scale readings unreliable during brew - switching to time-based stop");
      }
      setError(ERR_FLAG_SCALE);
    }
  } else {
    scaleConnected = false;
    // If scale becomes unavailable during brew, mark as unreliable
    if (brewActive && scaleReliableEntireShot) {
      scaleReliableEntireShot = false;
      Serial.println("Scale disconnected during brew - switching to time-based stop");
    }
    setError(ERR_FLAG_SCALE);
  }
}

void tareScale() {
  if (scaleInitialized && nau.available()) {
    scaleZeroOffset = nau.read();
    shotWeight = 0.0;
    Serial.println("Scale tared");
  }
}

void calculateFlowRate() {
  // Calculate flow rate during brewing only
  if (!brewActive || !scaleConnected) {
    currentFlowRate = 0.0;
    return;
  }

  // Update flow rate at regular intervals
  if (millisElapsed(lastFlowCalculation, FLOW_CALCULATION_INTERVAL)) {
    double weightDelta = shotWeight - lastWeightForFlow;
    double timeDelta = (millis() - lastFlowCalculation) / 1000.0;  // Convert to seconds

    if (timeDelta > 0 && weightDelta > 0) {
      // Convert weight delta (grams) to volume (ml) assuming espresso density ≈ 1g/ml
      // Calculate flow rate in ml/s
      currentFlowRate = weightDelta / timeDelta;

      // Apply smoothing to reduce noise (exponential moving average)
      const double flowAlpha = 0.3;  // Smoothing factor

      if (flowRateFirstCalculation) {
        smoothedFlowRate = currentFlowRate;
        flowRateFirstCalculation = false;
      } else {
        smoothedFlowRate = flowAlpha * currentFlowRate + (1.0 - flowAlpha) * smoothedFlowRate;
      }

      currentFlowRate = smoothedFlowRate;
    }

    // Update tracking variables
    lastWeightForFlow = shotWeight;
    lastFlowCalculation = millis();
  }
}

void calculateShotRatio() {
  // Calculate shot ratio (output÷dose) during brewing
  if (brewActive && scaleConnected && doseWeight > 0) {
    currentShotRatio = shotWeight / doseWeight;
  } else if (!brewActive) {
    // Preserve final ratio after brew ends, reset only on new brew
    // currentShotRatio remains at final value for display
  } else {
    currentShotRatio = 0.0;  // No scale or invalid dose weight
  }
}

// --- Fill System Functions ---
bool readFillProbe() {
  // Read fill probe analog value with hysteresis to prevent chattering
  int analogValue = analogRead(PIN_FILL_PROBE);

  // Smart fill probe hysteresis
  static bool probeStateEstablished = false;
  const int hysteresis = 20;  // ADC counts hysteresis band

  if (!probeStateEstablished) {
    // Initial state determination
    fillProbeWet = (analogValue > fillProbeThreshold + hysteresis);
    probeStateEstablished = true;
  } else {
    // Hysteresis logic to prevent chattering
    if (fillProbeWet && analogValue < fillProbeThreshold - hysteresis) {
      fillProbeWet = false;  // Transition to dry
    } else if (!fillProbeWet && analogValue > fillProbeThreshold + hysteresis) {
      fillProbeWet = true;  // Transition to wet
    }
    // No change if within hysteresis band
  }

  return fillProbeWet;
}

int readFillProbeRaw() {
  // Read raw analog value from fill probe for calibration
  return analogRead(PIN_FILL_PROBE);
}

void initFillSystem() {
  pinMode(PIN_FILL_PROBE, INPUT);
  pinMode(PIN_RELAY_CH3, OUTPUT);  // Fill Solenoid
  digitalWrite(PIN_RELAY_CH3, LOW);
  fillProbeWet = readFillProbe();
  Serial.printf("Fill system initialized. Probe: %s\n", fillProbeWet ? "WET" : "DRY");
}

// --- Pressure Measurement Functions ---
void initPressure() {
  // Initialize pressure transducer pin
  pinMode(PIN_PRESSURE, INPUT);

  // Pressure calibration values (pressureOffset, pressureScale) are loaded from EEPROM
  // in loadParametersFromEEPROM() during initEEPROM() call in setup()

  Serial.println("Pressure measurement system initialized");
}

double readPressure() {
  // Read analog value from pressure transducer
  int rawValue = analogRead(PIN_PRESSURE);

  // Convert to voltage (0-3.3V range) - Direct connection, no voltage divider
  double voltage = (rawValue * ADC_MAX_VOLTAGE) / ADC_RESOLUTION;

  // Convert voltage to pressure using calibration
  double pressure = (voltage - pressureOffset) * pressureScale;

  // Apply exponential moving average smoothing
  if (pressureFirstReading) {
    filteredPressure = pressure;
    pressureFirstReading = false;
  } else {
    filteredPressure = pressureAlpha * pressure + (1.0 - pressureAlpha) * filteredPressure;
  }

  // Update current pressure
  currentPressure = filteredPressure;

  // Pressure safety check with hysteresis (prevents pump oscillation)
  if (!pressureLockoutActive && currentPressure >= PRESSURE_LOCKOUT_HIGH) {
    // Enter pressure lockout state
    pressureLockoutActive = true;
    setError(ERR_FLAG_PRESSURE);
    Serial.printf("PRESSURE SAFETY: %.1f BAR exceeds %.1f BAR limit - entering lockout\n", currentPressure, PRESSURE_LOCKOUT_HIGH);

    // Stop brew cycle if active
    if (currentState == STATE_BREW_ACTIVE || currentState == STATE_BREW_START) {
      Serial.println("PRESSURE SAFETY: Stopping brew cycle");
      set_pump(false);
      set_brew_solenoid(false);
      changeState(STATE_BREW_END);
    }
  } else if (pressureLockoutActive && currentPressure <= PRESSURE_LOCKOUT_LOW) {
    // Exit pressure lockout state (hysteresis)
    pressureLockoutActive = false;
    clearError(ERR_FLAG_PRESSURE);
    Serial.printf("PRESSURE SAFETY: %.1f BAR below %.1f BAR - exiting lockout\n", currentPressure, PRESSURE_LOCKOUT_LOW);
  }

  return currentPressure;
}
