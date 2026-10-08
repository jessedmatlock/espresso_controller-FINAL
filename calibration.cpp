// calibration.cpp — fill probe, scale, and pressure calibration flows.
// Extracted verbatim; no logic changes. Single task only.
#include <Arduino.h>
#include <Adafruit_NAU7802.h>
#include "config.h"
#include "state_machine.h"
#include "sensors.h"
#include "relays.h"
#include "calibration.h"

CalStep calStep = CAL_IDLE;

// Hardware object — defined in the main .ino.
extern Adafruit_NAU7802 nau;

// Helper defined in the main .ino.
void recomputeSetupComplete();

// --- Fill probe calibration state (shared with sensors.cpp/web_api.cpp/eeprom_store.cpp) ---
extern int fillProbeDryReading;
extern int fillProbeWetReading;
extern int fillProbeThreshold;
extern bool fillCalComplete;

// --- Scale calibration state ---
extern bool scaleInitialized;
extern float scaleCalibrationFactor;
extern int32_t scaleZeroOffset;
extern int32_t scaleZeroReading;
extern int32_t scaleKnownWeightReading;
extern bool scaleCalComplete;

// --- Pressure calibration state ---
extern bool pressureCalComplete;
extern int pressureZeroReading;
extern int pressureHighReading;
extern double pressureHighValue;
extern double pressureOffset;
extern double pressureScale;

// --- Setup-flow flags ---
extern bool setupComplete;
extern bool eepromDirty;
extern SystemState currentState;

const char* calStepName(CalStep s) {
  switch (s) {
    case CAL_FILL_DRY:      return "FILL_DRY";
    case CAL_FILL_WET:      return "FILL_WET";
    case CAL_SCALE_ZERO:    return "SCALE_ZERO";
    case CAL_SCALE_WEIGHT:  return "SCALE_WEIGHT";
    case CAL_SCALE_CONFIRM: return "SCALE_CONFIRM";
    case CAL_PRESSURE_ZERO: return "ZERO";
    case CAL_PRESSURE_HIGH: return "HIGH";
    default:                return "";
  }
}

void startFillProbeCalibration() {
  if (currentState != STATE_IDLE) {
    Serial.println("Cannot calibrate - system not in IDLE state");
    return;
  }

  calStep = CAL_FILL_DRY;
  Serial.println("Fill probe calibration started - ensure probe is DRY");
}

void startScaleCalibration() {
  if (currentState != STATE_IDLE) {
    Serial.println("Cannot calibrate - system not in IDLE state");
    return;
  }

  scaleCalComplete = false;
  recomputeSetupComplete();
  eepromDirty = true;
  calStep = CAL_SCALE_ZERO;
  Serial.println("Scale calibration started - remove all weight from scale");
}

void processCalibrationStep() {
  if (!isFillScaleCal()) return;

  switch (calStep) {
    case CAL_FILL_DRY:
      fillProbeDryReading = readFillProbeRaw();
      Serial.printf("Fill probe DRY reading: %d\n", fillProbeDryReading);
      calStep = CAL_FILL_WET;
      Serial.println("Now ensure probe is WET (immerse in water)");
      break;

    case CAL_FILL_WET:
      fillProbeWetReading = readFillProbeRaw();
      Serial.printf("Fill probe WET reading: %d\n", fillProbeWetReading);
      fillProbeThreshold = (fillProbeDryReading + fillProbeWetReading) / 2;
      Serial.printf("Fill probe threshold set to: %d\n", fillProbeThreshold);
      fillCalComplete = true;
      calStep = CAL_IDLE;
      eepromDirty = true;
      Serial.println("Fill probe calibration completed and saved to EEPROM");
      break;

    case CAL_SCALE_ZERO:
      if (scaleInitialized && nau.available()) {
        scaleZeroReading = nau.read();
        Serial.printf("Scale ZERO reading: %.2f\n", (float)scaleZeroReading);
        calStep = CAL_SCALE_WEIGHT;
        Serial.println("Now place known weight (100g or 200g) on scale");
      }
      break;

    case CAL_SCALE_WEIGHT:
      if (scaleInitialized && nau.available()) {
        scaleKnownWeightReading = nau.read();
        Serial.printf("Scale WEIGHT reading: %.2f\n", (float)scaleKnownWeightReading);
        calStep = CAL_SCALE_CONFIRM;
        Serial.println("Enter known weight value to complete calibration");
      }
      break;

    default: break;
  }
}

void completeScaleCalibration(float knownWeight) {
  if (calStep == CAL_SCALE_CONFIRM) {
    float rawRange = scaleKnownWeightReading - scaleZeroReading;
    if (rawRange != 0 && knownWeight > 0) {
      scaleCalibrationFactor = rawRange / knownWeight;
      scaleZeroOffset = scaleZeroReading;  // Set zero offset for tare functionality

      Serial.printf("Scale calibration completed:\n");
      Serial.printf("  Zero reading: %.2f\n", (float)scaleZeroReading);
      Serial.printf("  Weight reading: %.2f\n", (float)scaleKnownWeightReading);
      Serial.printf("  Known weight: %.1fg\n", knownWeight);
      Serial.printf("  Calibration factor: %.4f\n", scaleCalibrationFactor);
      Serial.printf("  Zero offset: %ld\n", scaleZeroOffset);

      // Mark scale calibration as complete (optional, non-blocking to PID)
      scaleCalComplete = true;
      recomputeSetupComplete();

      calStep = CAL_IDLE;

      eepromDirty = true;
    } else {
      Serial.println("Invalid calibration values");
    }
  }
}

void startPressureCalibration() {
  if (currentState != STATE_IDLE) {
    Serial.println("Cannot start pressure calibration - system not in IDLE state");
    return;
  }

  if (isPressureCal()) {
    Serial.println("Pressure calibration already in progress");
    return;
  }

  // Start pressure calibration mode
  calStep = CAL_PRESSURE_ZERO;
  pressureCalComplete = false;
  recomputeSetupComplete();
  eepromDirty = true;

  // Reset accumulators to prevent stale values from previous calibration attempts
  pressureZeroReading = 0;
  pressureHighReading = 0;

  Serial.println("Pressure calibration started - ensure system is at ZERO pressure");
  Serial.println("FILL and BREW operations are now locked out");
  Serial.println("Press button when ready to calibrate ZERO point");
}

void processPressureCalibrationStep() {
  if (!isPressureCal()) return;

  // Collect all 4 oversamples in one call (called only from web handler, not main loop)
  const int OVERSAMPLE_COUNT = 4;
  const unsigned long SAMPLE_DELAY_MS = 10;

  if (calStep == CAL_PRESSURE_ZERO) {
    // Sample zero pressure reading with oversampling
    pressureZeroReading = 0;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
      pressureZeroReading += analogRead(PIN_PRESSURE);
      if (i < OVERSAMPLE_COUNT - 1) delay(SAMPLE_DELAY_MS);  // 10ms between samples, 30ms total
    }
    pressureZeroReading /= OVERSAMPLE_COUNT;
    Serial.printf("Zero pressure reading: %d\n", pressureZeroReading);

    // Move to high pressure calibration
    calStep = CAL_PRESSURE_HIGH;
    Serial.printf("Now use Web UI 'Start Pump (Cal)' button and set actual pressure to %.1f BAR\n", pressureHighValue);
    Serial.println("Use 'Set Pressure Value' to confirm reading, then 'Next Pressure Step' to complete");

  } else if (calStep == CAL_PRESSURE_HIGH) {
    // Sample high pressure reading with oversampling
    pressureHighReading = 0;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
      pressureHighReading += analogRead(PIN_PRESSURE);
      if (i < OVERSAMPLE_COUNT - 1) delay(SAMPLE_DELAY_MS);
    }
    pressureHighReading /= OVERSAMPLE_COUNT;
    Serial.printf("High pressure reading: %d\n", pressureHighReading);

    // Calculate calibration values
    completePressureCalibration();
  }
}

void completePressureCalibration() {
  // Minimum voltage range for valid calibration (prevents divide-by-zero and noise issues)
  const double MIN_CAL_VOLTAGE_RANGE = 0.5;  // 0.5V minimum range required

  // Convert ADC counts to voltages for consistent unit handling
  double zeroVoltage = (pressureZeroReading * ADC_MAX_VOLTAGE) / ADC_RESOLUTION;
  double highVoltage = (pressureHighReading * ADC_MAX_VOLTAGE) / ADC_RESOLUTION;
  double voltageRange = highVoltage - zeroVoltage;

  // Validate calibration (minimum voltage range for reliable calibration)
  if (voltageRange > MIN_CAL_VOLTAGE_RANGE) {
    // Calculate voltage per bar (now in proper voltage units)
    double voltagePerBar = voltageRange / pressureHighValue;

    // Set calibration values (all in voltage units for consistency with readPressure())
    pressureOffset = zeroVoltage;  // Store as voltage, not ADC counts
    pressureScale = 1.0 / voltagePerBar;  // BAR per volt

    Serial.printf("Pressure calibration completed:\n");
    Serial.printf("  Zero reading: %d (%.3fV)\n", pressureZeroReading, zeroVoltage);
    Serial.printf("  High reading: %d (%.3fV)\n", pressureHighReading, highVoltage);
    Serial.printf("  Voltage range: %.3fV\n", voltageRange);
    Serial.printf("  Offset: %.3fV\n", pressureOffset);
    Serial.printf("  Scale: %.3f BAR/V\n", pressureScale);

    // Mark pressure calibration as complete
    pressureCalComplete = true;

    // Update setupComplete if all required calibrations are done
    recomputeSetupComplete();

    eepromDirty = true;

    // Stop pump and exit calibration mode
    set_pump(false);
    calStep = CAL_IDLE;

    Serial.println("Pressure calibration completed successfully - pump stopped");
    Serial.println("FILL and BREW operations are now unlocked");
    if (setupComplete) {
      Serial.println("All required calibrations complete - PID enabled");
    }

  } else {
    // Calibration failed - insufficient voltage range
    Serial.println("ERROR: Pressure calibration failed - insufficient voltage range");
    Serial.printf("Voltage range: %.3fV (minimum required: %.1fV)\n",
                  voltageRange, MIN_CAL_VOLTAGE_RANGE);
    Serial.println("Check pressure transducer wiring and ensure proper pressure range");

    // Keep previous calibration values
    Serial.println("Previous calibration values retained");

    // Stop pump and exit calibration mode
    set_pump(false);
    calStep = CAL_IDLE;
    Serial.println("FILL and BREW operations are now unlocked");
  }
}
