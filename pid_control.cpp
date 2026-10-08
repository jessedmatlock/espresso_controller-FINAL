// pid_control.cpp — PID temperature control. Same math as the original
// pid_step()/checkPIDTiming(), unchanged — the only thing removed is the
// FreeRTOS task wrapper (pidTask()/initPIDTask()) and the pidMux critical
// sections, both of which existed solely to make this safe to call from a
// second CPU core. With a single task running everything, there is no
// concurrent caller to guard against: pid_integral/pid_prev_error are now
// plain statics, exactly as safe as the `static double lastPidTemp` already
// sitting a few lines below them always was.
#include <Arduino.h>
#include "config.h"
#include "error_system.h"
#include "shared_state.h"
#include "state_machine.h"
#include "pid_control.h"

// --- Functions defined elsewhere ---
bool isPidAllowed();                                       // main .ino
void set_boiler_element(bool state);                        // relays.cpp
void set_pump(bool state);                                   // relays.cpp
bool millisElapsed(unsigned long startTime, unsigned long interval);  // main .ino

// --- Performance-monitoring variables: owned by the main .ino, shared with
// checkMemoryUsage()/logPerformanceMetrics()/processPIDTaskLogs() and the
// PERF_* serial commands. Still declared extern (not merged into this file)
// since they're genuinely about the whole system's performance, not just
// PID — unrelated to the single-task change.
extern unsigned long lastPIDTime;
extern unsigned long maxPIDJitter;
extern unsigned long pidJitterCount;
extern bool performanceMonitoringEnabled;
extern bool pidTimingWarningActive;
extern bool pidLogTempRateError;
extern double pidLogTempRate;
extern bool pidLogTimingWarning;
extern unsigned long pidLogJitter;
extern unsigned long pidLogJitterCount;
extern bool pidLogTimingNormalized;

// Private to this file. No longer volatile or mutex-guarded — single task,
// no concurrent access is possible.
static double pid_integral = 0.0;
static double pid_prev_error = 0.0;

void resetPidIntegral() {
  pid_integral = 0.0;
}

// --- Performance Monitoring (PID cycle timing) ---
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

// --- PID Control Function (called once per CONTROL_INTERVAL_MS) ---
static void pid_step() {
  // Performance monitoring - PID timing check
  checkPIDTiming();

  // Safety override FIRST - stop heating/pumping for critical errors before any computation
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

  // Use the cached raw temperature (refreshed every ~50ms elsewhere) rather
  // than reading the RTD directly here — unchanged from the original design.
  // This keeps the EMA filter below fed at the same cadence it always was;
  // changing that cadence would change the filter's effective behavior.
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
        // Set flag for main loop to log (keeps Serial access off the hot path)
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

  // Publish filtered temp for display/JSON/web
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
  double snapKp = getKp();
  double snapKi = getKi();
  double snapKd = getKd();

  pid_integral += error * (CONTROL_INTERVAL_MS / 1000.0);

  // PID integral windup protection
  if (pid_integral > integralMax) pid_integral = integralMax;
  if (pid_integral < -integralMax) pid_integral = -integralMax;

  // Use derivative on measurement (not error) to prevent "derivative kick" on setpoint changes
  // Negative sign because we're measuring temperature change, not error change
  // Uses prevPidTemp (stored before update) to get actual temperature rate of change
  double derivative = -(filteredTemp - prevPidTemp) / (CONTROL_INTERVAL_MS / 1000.0);
  double pidOutput = snapKp * error + snapKi * pid_integral + snapKd * derivative;
  // Note: pid_prev_error kept for potential future use, but derivative now uses measurement
  pid_prev_error = error;

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

void updatePidControl() {
  static unsigned long lastPidRun = 0;
  if (!millisElapsed(lastPidRun, CONTROL_INTERVAL_MS)) return;
  lastPidRun = millis();

  if (!isPidAllowed()) {
    // Setup not complete - disable PID heating, only read temperature
    set_boiler_element(false);
    setCurrentPIDOutputPercent(0.0);
    resetPidIntegral();
    setTemp(getCachedRawTemp());
    return;
  }

  pid_step();
}
