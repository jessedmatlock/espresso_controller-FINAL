// control_task.cpp — Core 1 PID control task. See control_task.h for the
// core-ownership contract. Extracted from the original pidTask()/pid_step()/
// initPIDTask() with identical logic; the only behavioral change is that
// Kp/Ki/Kd and boilerHeating now flow through shared_state.h's mutex-
// protected accessors instead of bare globals (see shared_state.cpp).
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "error_system.h"
#include "shared_state.h"
#include "state_machine.h"
#include "control_task.h"

// --- Functions defined elsewhere, called from this task ---
// isPidAllowed(): main .ino (depends on setupComplete, Core-0-owned).
// set_boiler_element()/set_pump(): relays.cpp.
// millisElapsed(): main .ino (pure helper, no shared state).
bool isPidAllowed();
void set_boiler_element(bool state);
void set_pump(bool state);
bool millisElapsed(unsigned long startTime, unsigned long interval);

// --- Performance-monitoring variables: still owned by the main .ino
// (shared with checkMemoryUsage()/logPerformanceMetrics()/processPIDTaskLogs()
// on Core 0, and the PERF_* serial commands) — unchanged, not part of this
// refactor's scope. Declared extern here since checkPIDTiming()/pid_step()
// read and write them from Core 1, exactly as before.
extern volatile unsigned long lastPIDTime;
extern volatile unsigned long maxPIDJitter;
extern volatile unsigned long pidJitterCount;
extern volatile bool performanceMonitoringEnabled;
extern volatile bool pidTimingWarningActive;
extern volatile bool pidLogTempRateError;
extern volatile double pidLogTempRate;
extern volatile bool pidLogTimingWarning;
extern volatile unsigned long pidLogJitter;
extern volatile unsigned long pidLogJitterCount;
extern volatile bool pidLogTimingNormalized;

// --- Core 1 task internals — private to this file ---
static TaskHandle_t pidTaskHandle = NULL;
static volatile bool pidTaskRunning = false;
static portMUX_TYPE pidMux = portMUX_INITIALIZER_UNLOCKED;
static volatile double pid_integral = 0.0;
static volatile double pid_prev_error = 0.0;

void resetPidIntegral() {
  portENTER_CRITICAL(&pidMux);
  pid_integral = 0.0;
  portEXIT_CRITICAL(&pidMux);
}

// --- Performance Monitoring (PID timing only; memory/web metrics stay on Core 0) ---
static void checkPIDTiming() {
  if (!performanceMonitoringEnabled) return;

  unsigned long currentTime = millis();

  if (lastPIDTime > 0) {
    unsigned long pidJitter = currentTime - lastPIDTime;

    // Track maximum jitter
    if (pidJitter > maxPIDJitter) {
      maxPIDJitter = pidJitter;
    }

    // Alert on excessive jitter (set flag for main loop to log)
    if (pidJitter > PID_JITTER_THRESHOLD) {
      pidJitterCount++;
      if (!pidTimingWarningActive) {
        pidLogJitter = pidJitter;
        pidLogJitterCount = pidJitterCount;
        pidLogTimingWarning = true;
        pidTimingWarningActive = true;
      }
    } else if (pidTimingWarningActive && pidJitter <= 102) {
      // Clear warning when timing returns to normal
      pidTimingWarningActive = false;
      pidLogTimingNormalized = true;
    }
  }

  lastPIDTime = currentTime;
}

// --- PID Control Function (runs every 100ms on the ControlTask) ---
static void pid_step() {
  // Performance monitoring - PID timing check
  checkPIDTiming();

  // Safety override FIRST - stop heating/pumping for critical errors before any computation
  // Use atomic error flags for thread-safe access from Core 1
  if (hasCriticalErrorFlag()) {
    setCurrentPIDOutputPercent(0.0);
    set_boiler_element(false);
    if (hasErrorFlag(ERR_FLAG_TEMP)) {
      set_pump(false);
    }
    // Still update display temp from cache so UI stays current during error
    setTemp(getCachedRawTemp());
    return;
  }

  // Use cached raw temperature from main loop (avoids SPI access from Core 1)
  double temp = getCachedRawTemp();

  // Temperature rate-of-change safety check (uses unified MAX_TEMP_RATE constant)
  static double lastPidTemp = 77.0;
  static unsigned long lastPidTempTime = 0;

  unsigned long now = millis();
  if (lastPidTempTime > 0) {
    double timeDiff = (now - lastPidTempTime) / 1000.0;  // seconds
    if (timeDiff > 0) {
      double tempRate = abs(temp - lastPidTemp) / timeDiff;
      if (tempRate > MAX_TEMP_RATE) {
        // Set flag for main loop to log (avoid Serial from this task)
        pidLogTempRate = tempRate;
        pidLogTempRateError = true;
        setErrorFlag(ERR_FLAG_TEMP);
        return;  // Exit PID control - use last valid temperature
      }
    }
  }

  // Store previous temperature BEFORE updating for derivative calculation
  double prevPidTemp = lastPidTemp;
  lastPidTemp = temp;
  lastPidTempTime = now;

  // Exponential moving average temperature filtering using TEMP_FILTER_ALPHA constant
  static double filteredTemp = 77.0;  // Initialize to room temperature
  static bool firstReading = true;

  if (firstReading) {
    filteredTemp = temp;
    firstReading = false;
  } else {
    filteredTemp = TEMP_FILTER_ALPHA * temp + (1.0 - TEMP_FILTER_ALPHA) * filteredTemp;
  }

  // Publish filtered temp for Core 0 (display/JSON) via shared_state
  setTemp(filteredTemp);

  // PID Boost override during brew (100% heater for configurable duration)
  // Temperature guard: only apply boost if below setpoint + 5°F to prevent overshoot
  const double PID_BOOST_TEMP_MARGIN = 5.0;
  if (getPidBoostActive() && getState() == STATE_BREW_ACTIVE &&
      filteredTemp < (getSetpointTemp() + PID_BOOST_TEMP_MARGIN)) {
    setCurrentPIDOutputPercent(100.0);  // Show 100% during PID boost
    set_boiler_element(true);
    return;
  }

  double error = getSetpointTemp() - filteredTemp;

  // Snapshot gains before the pidMux critical section — avoids nesting the
  // inputsMux (inside getKp/getKi/getKd) inside pidMux.
  double snapKp = getKp();
  double snapKi = getKi();
  double snapKd = getKd();

  double pidOutput;
  portENTER_CRITICAL(&pidMux);
  pid_integral += error * (CONTROL_INTERVAL_MS / 1000.0);

  // PID integral windup protection
  if (pid_integral > integralMax) pid_integral = integralMax;
  if (pid_integral < -integralMax) pid_integral = -integralMax;

  // Use derivative on measurement (not error) to prevent "derivative kick" on setpoint changes
  // Negative sign because we're measuring temperature change, not error change
  // Uses prevPidTemp (stored before update) to get actual temperature rate of change
  double derivative = -(filteredTemp - prevPidTemp) / (CONTROL_INTERVAL_MS / 1000.0);
  pidOutput = snapKp * error + snapKi * pid_integral + snapKd * derivative;
  // Note: pid_prev_error kept for potential future use, but derivative now uses measurement
  pid_prev_error = error;
  portEXIT_CRITICAL(&pidMux);

  // Map pidOutput to 0..1 via clamp and scaling
  // PID_OUTPUT_SCALE of 1000.0 means:
  // - With Kp=40, a 25°F error produces P-term = 1000 = 100% output
  // - With Ki=0.8 and max integral=1000, I-term maxes at 800 = 80% output
  // - With Kd=120, a 5°F/s rate produces D-term = 600 = 60% output
  const double PID_OUTPUT_SCALE = 1000.0;
  double out = pidOutput / PID_OUTPUT_SCALE;
  if (out > 1.0) out = 1.0;
  if (out < 0.0) out = 0.0;

  // Update PID output percentage for display
  setCurrentPIDOutputPercent(out * 100.0);

  // Time-proportional burst-fire control (fixed 1s window)
  const unsigned long window_ms = 1000;  // 1s window as required
  static unsigned long window_start = millis();
  if (millisElapsed(window_start, window_ms)) window_start = millis();

  // Bounds checking to prevent overflow from negative PID output
  if (out < 0.0) out = 0.0;  // Prevent negative values from integral windup
  if (out > 1.0) out = 1.0;  // Prevent output > 100%

  unsigned long on_time = (unsigned long)(out * window_ms);

  // Single state change per window
  if (!millisElapsed(window_start, on_time)) set_boiler_element(true);
  else set_boiler_element(false);
}

// --- Core 1 ControlTask ---
static void pidTask(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_INTERVAL_MS);  // 100ms

  pidTaskRunning = true;
  Serial.printf("ControlTask started on core %d\n", xPortGetCoreID());
  // Register this task with the watchdog to catch stalls
  if (esp_task_wdt_add(NULL) != ESP_OK) {
    Serial.println("WARNING: Failed to add ControlTask to watchdog");
  }

  for (;;) {
    // Wait for next cycle (precise timing with vTaskDelayUntil)
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    // Feed watchdog for this task
    esp_task_wdt_reset();

    // Check if required calibrations are complete before running PID
    if (!isPidAllowed()) {
      // Setup not complete - disable PID heating, only read temperature
      set_boiler_element(false);
      setCurrentPIDOutputPercent(0.0);

      // Reset PID integral to prevent windup accumulation during setup
      resetPidIntegral();

      // Use cached temp for display (main loop handles SPI reads)
      setTemp(getCachedRawTemp());
      continue;
    }

    // Execute PID control (publishes filteredTemp/pidOutputPercent/boilerHeating
    // via shared_state.h for Core 0 to read)
    pid_step();
  }
}

void initPIDTask() {
  // Create dedicated PID task on Core 1 with highest priority
  // Priority 24 is highest for application tasks (configMAX_PRIORITIES - 1)
  // Stack size 6144 bytes provides margin for PID calculations and safety checks
  BaseType_t result = xTaskCreatePinnedToCore(
    pidTask,         // Task function
    "ControlTask",   // Task name
    6144,            // Stack size (bytes) - increased for safety margin
    NULL,            // Parameters
    24,              // Priority (highest for real-time control)
    &pidTaskHandle,  // Task handle
    1                // Core 1 (dedicated to PID) — explicit, never assumed
  );

  if (result != pdPASS) {
    Serial.println("ERROR: Failed to create ControlTask on Core 1");
    return;
  }

  Serial.println("ControlTask created on Core 1 (100ms interval, highest priority)");
}
