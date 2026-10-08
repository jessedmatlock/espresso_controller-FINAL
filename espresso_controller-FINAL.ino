/*
Arduino Nano ESP32 Espresso Controller - Arduino sketch
Target: Arduino Nano ESP32 (NORA-W106 ESP32-S3 core)

SERIAL COMMANDS 
================
Baud rate: 115200 | Rate limit: 100ms between commands | Case-sensitive


SETPOINT=200.0
TARGET_TIME=30.0
TARGET_WEIGHT=36.0
PID_BOOST=4.0
PREINFUSION=8.0
PID_BYPASS=0    — 0 off, 1 on - Allows PID to run before setupComplete. Persisted to EEPROM. Use with caution — only safe when hardware is stable.
CLEANING_CYCLES=10    - 1-20, Sets number of 10s ON / 10s OFF pump cycles. Auto-saves.
TARE            — zero scale once load cell is stable
SAVE            -Forces immediate EEPROM write (bypasses the normal 2-second deferred save).
LOAD            - Reloads all parameters from EEPROM, discarding any unsaved changes in RAM.
STATUS          — Single-line snapshot: Temp, SP, Boiler relay, Pump relay, Brew solenoid, Fill solenoid, Weight, Scale state
PERF_ON         — start monitoring PID timing and memory from first boot
PERF_OFF        - Silences all performance monitoring output.
PERF_RESET      - Clears all counters: PID jitter, heap minimum, web request stats, WebSocket count.
PERF_REPORT
  === PERFORMANCE METRICS ===
  PID Timing: Max jitter Xms, Warning count: Y
  Memory: Current X bytes, Minimum Y bytes
  Web Requests: Total X, Avg response: Yms
  WebSocket Messages: Z sent
  Active Warnings: PID=YES/NO, Memory=YES/NO
  ==========================



REVISION 64 - Current Status:
- Rev 0: Baseline code (original)
- Rev 1: Added OLED display interface (SH1107 128x128)
- Rev 2: Added HX711 load cell interface for shot weight measurement
- Rev 3: Added fill level probe interface and fill routine
- Rev 4: Added brew switch interface (12V via voltage divider) - originally toggle, later converted to momentary operation
- Rev 5: Replaced SSR/pump pins with 4-channel relay control
- Rev 6: Converted all temperature handling to Fahrenheit (200°F default, 225°F safety)
- Rev 7: Moved PID control to Core 1 hardware timer ISR firing every 100ms
- Rev 8: Confirmed 1s PID window with single state change (already implemented in Rev 7)
- Rev 9: Implemented formal state machine (BOOT, IDLE, BREW_START, BREW_ACTIVE, BREW_END, FILL, ERROR)
- Rev 10: Added shot timing and weight-based brew control with stop conditions
- Rev 11: Added PID Boost and Pre-infusion functionality with configurable durations
- Rev 12: Added EEPROM storage structure with signature and all configurable parameters
- Rev 13: Added JSON status endpoint and real-time Web UI with Chart.js integration
- Rev 14: Added comprehensive error handling system with persistent states and auto-recovery
- Rev 15: Replaced placeholder with actual MAX31865 PT100 sensor implementation (3-wire)
- Rev 16: Added calibration flows for fill probe and scale via Web UI
- Rev 17: Eliminated delay() calls and implemented non-blocking millis()-based timing
- Rev 18: Implemented Core 0/Core 1 task separation with proper FreeRTOS task creation
- Rev 19: DEVIATION FIXES - Added missing MAX31865 include, fixed architecture to requirements-compliant design, improved safety margins, added complete EEPROM persistence, implemented parameter validation, and optimized display performance
- Rev 20: PRODUCTION OPTIMIZATIONS - Added PID integral windup protection, temperature filtering, scale stability validation, predictive temperature compensation, fill probe hysteresis, adaptive display rates, Web UI compression, and memory pool management
- Rev 21: CRITICAL ARCHITECTURE FIXES - Added pressure measurement system, eliminated remaining delay() calls from setup, replaced with fully non-blocking architecture, added fill timeout protection, rollover-safe timing, and calibration state protection
- Rev 22: PRESSURE MEASUREMENT IMPLEMENTATION - Added complete pressure reading, calibration, and state management functions with non-blocking operation
- Rev 23: PRESSURE UI INTEGRATION - Added pressure display to OLED and web UI, Chart.js graphing, calibration workflow, and EEPROM persistence
- Rev 24: PRESSURE CALIBRATION STATE MANAGEMENT - Added complete lockout of FILL and BREW during pressure calibration with proper logging and user messaging
- Rev 25: MOMENTARY BREW BUTTON LOGIC - Implemented rising edge detection, double-press protection, state-aware brew start/stop, and error state logging
- Rev 26: MILLIS ROLLOVER PROTECTION - Applied millisElapsed() to all critical timing operations including PID control, brew timing, fill logging, and main loop intervals
- Rev 27: REQUIREMENTS COMPLETION - Added SSD1327 SPI pin assignments with optimal routing, updated toggle→momentary switch documentation, implemented user pump control for pressure calibration HIGH step
- Rev 28: BREW SEQUENCE FIXES - Shot timer and weight persist on display after brew end, brew start resets timer and auto-tares scale, PID boost confirmed as 100% heater override
- Rev 29: FILL SEQUENCE ENHANCEMENT - Optimized fill cycles: solenoid and pump both ON during 2s cycles, both OFF while reading probe for accurate sensing
- Rev 30: CRITICAL SAFETY FIXES - Added PID output bounds checking to prevent overflow, renamed software watchdog to LOOP_TIMEOUT to avoid hardware watchdog naming conflict
- Rev 31: USER-CONFIGURABLE PRESSURE CALIBRATION - Added Web UI input field for actual pressure value during calibration, replaced hardcoded 8 BAR with user-specified value (1-15 BAR range)
- Rev 32: SCALE ERROR DISPLAY SYSTEM - Added system message display on OLED bottom line and Web UI banner, shows "SCALE ERR" when scale disconnects while retaining stale weight data
- Rev 33: EEPROM OPTIMIZATION COMPLETE - All calibrations save to EEPROM only on successful completion, preventing excessive wear from intermediate steps, enhanced fill calibration persistence
- Rev 34: FINAL PRODUCTION POLISH - Fixed fill cycle timing (solenoid/pump both ON only during cycles, both OFF for accurate probe reading), completed production code optimization
- Rev 35: RELIABILITY ENHANCEMENTS - Added initialization failure detection and recovery (OLED, I2C, Web Server), implemented mutex protection for scale calibration operations to prevent race conditions
- Rev 36: DEVELOPMENT DOCUMENTATION - Added comprehensive serial command reference and complete EEPROM memory map for development and maintenance
- Rev 37: UNIFIED 100MS UPDATE RATE - Synchronized OLED and Web UI to 100ms for perfect timing consistency, optimized performance
- Rev 38: ARDUINO NANO ESP32 PORT - Complete pin remapping for Arduino Nano ESP32, optimized consecutive pin grouping (A0-A3 inputs, D5-D8 outputs), updated all pin documentation
- Rev 39: BREW SWITCH UPDATE - Modified for simple NO momentary switch (GPIO to GND), enabled internal pullup, inverted logic for LOW-when-pressed operation
- Rev 40: SYNCHRONIZED 100MS DISPLAY UPDATES - Unified OLED and Web UI timing for perfect synchronization, enhanced real-time accuracy for timer, weight, temperature, and pressure measurements
- Rev 41: MASTER PLAN OPTIMIZATIONS - WebSocket implementation for real-time Web UI updates, memory pool optimization with pre-allocated buffers, loop() function refactoring for improved code maintainability
- Rev 42: COMPLETE MASTER PLAN IMPLEMENTATION - Scale reading optimization (50ms during brew), WebSocket real-time updates, memory pool optimization, pressure safety cutoff (12+ BAR), loop() function refactoring, scale validation, flow rate display, shot ratio tracking, pressure profile visualization, mobile responsive design, status dashboard enhancement, advanced controls panel, dynamic chart scaling with EEPROM targets
- Rev 43: CLEANING CYCLE IMPLEMENTATION - Added automated cleaning cycle with 10 cycles of 10s pump ON/10s pump OFF, Web UI controls for start/stop, OLED and Web UI status display, brew button mechanical backup, brew protection during cleaning, pressure chart display during cleaning, comprehensive error handling and state management
- Rev 44: CLEANING CYCLE SAFETY & OPTIMIZATION - Fixed critical memory leak in getStateName(), added pump timeout detection (15s/3 failures), 5-minute safety limit, pre-cleaning fill validation, blocked fill during cleaning, improved chart management, WebSocket button synchronization, completion feedback, reduced serial logging
- Rev 45: CLEANING CYCLE FINAL FIXES - Fixed pump timeout logic to stop pump on failure, added cleaningActive to individual WebSocket clients, completion message auto-clear after 5 seconds, pump failure count reset on manual restart
- Rev 46: PERFORMANCE OPTIMIZATIONS - Increased WebSocket buffer to 512 bytes (prevents overflow), added pump timeout race condition protection, optimized completion message handling with boolean flag
- Rev 47: BOOTSTRAP 5 INTEGRATION - Added Bootstrap 5.3.0 CSS and JS via PROGMEM header files, converted layout to responsive grid system (Grid, Cards, Buttons, Forms, Badge, Navs, Tabs, Offcanvas, Progress), enhanced mobile responsiveness with touch-friendly components, maintained Chart.js and WebSocket functionality
- Rev 48: NAU7802 MIGRATION - Replaced HX711 with NAU7802 24-bit ADC for load cell (I2C interface, shares bus with OLED), configured for 80 SPS/GAIN_128/3V3 excitation, implemented manual calibration math, added persistent zero offset to EEPROM (address 58-61), freed GPIO6/GPIO7 pins, updated EEPROM total to 62 bytes
- Rev 49: CODE REVIEW FIXES - Fixed PIN_TEMP_CS declaration order, removed duplicate now variable, fixed undefined PIN_FILL_SOLENOID, migrated to esp_timer API, fixed invalid portTRY_ENTER_CRITICAL, converted flow rate/scale validation statics to globals with proper reset, fixed String concatenation heap fragmentation, updated stale HX711 comment, added ESP-IDF 5.x watchdog API compatibility
- Rev 50: FINAL SAFETY & ARCHITECTURE REVIEW - Removed debug watchdog code, added state change mutex protection, removed redundant temperature read in PID, implemented over-temperature auto-recovery (PID resumes after temp drops 10°F below threshold), implemented pressure safety lockout (pump blocked >12 BAR until <11 BAR), migrated PID to dedicated FreeRTOS task on Core 1 with highest priority and vTaskDelayUntil for deterministic 100ms timing, added first-run setup system with EEPROM flags (tempCalComplete, pressureCalComplete, setupComplete), added first-run OLED display (SETUP REQ./espresso_AP/192.168.4.1/PID: OFF), added firstRun/tempCalComplete/pressureCalComplete flags to WebSocket and Status API, refactored Web UI into PROGMEM header files (web_ui_css.h, web_ui_html.h, web_ui_js.h), updated EEPROM to 65 bytes (addresses 62-64 for setup flags)
- Rev 51: SAFETY & PERFORMANCE ENHANCEMENTS - Added boilerFilled to setup requirements (EEPROM addr 65, 66 bytes total), changed temp calibration to require wizard completion (removed auto-set on init, added /calibrate/temp/complete endpoint), added tempMux/cachedTempMux for thread-safe temperature access, moved SPI temp read from PID task to main loop (50ms cache update), PID task now uses getCachedRawTemp() to avoid Core 1 SPI access, increased PID task stack to 6144 bytes, removed Serial.printf from PID task (flag-based logging via processPIDTaskLogs() in main loop), added boilerFilled to WebSocket and Status API
- Rev 52: THREAD SAFETY FINAL - Fixed errorMessage race condition by adding atomic error flags (ERR_FLAG_TEMP, ERR_FLAG_RTD, ERR_FLAG_PID, ERR_FLAG_SCALE, ERR_FLAG_PRESSURE) for thread-safe PID task access, added hasCriticalErrorFlag() and hasErrorFlag() inline helpers, updated pid_step() to use atomic flags instead of String-based checks, updated EEPROM memory map documentation to reflect actual 66-byte layout with all setup flags
- Rev 53: SIMPLIFIED FIRST-RUN - Temperature probe now auto-confirms on successful initialization (no manual wizard step required), validates no RTD faults and reading in 32-250°F range, reduces user friction while maintaining safety, /calibrate/temp/complete endpoint retained as manual override for edge cases
- Rev 54: DUAL BOILER ARCHITECTURE - Separated steam boiler (pressostat-controlled) from brew boiler (PID-controlled), removed boilerFilled from PID requirements (setupComplete = tempCalComplete && pressureCalComplete only), added runStartupFillCycle() that runs during setup() to ensure steam boiler has water before pressostat heater activates, startup fill has 2-minute timeout with watchdog feeding, boilerFilled now tracks steam boiler state independently of PID operation
- Rev 55: SAFETY & PERFORMANCE REVIEW - Removed relay toggle test in setup() (S-1), enforced 250ms minimum pre-infusion pulse rate to protect pump/relay (S-2), added critical section to emergencyStop() for atomic state changes (S-3), added brew/fill solenoid interlocks (S-4), unified temperature rate limit constant MAX_TEMP_RATE=5°F/s (S-5), changed PID to derivative-on-measurement to prevent setpoint change kick (P-1), documented PID_OUTPUT_SCALE constant (P-2), increased WebSocket buffer to 768 bytes (P-3), moved scale stability variables to global scope (P-4), reset pressure calibration accumulators at start (P-5)
- Rev 56: PID DERIVATIVE FIX - Fixed critical bug where lastPidTemp was updated before being used in derivative calculation, causing D-term to always be near zero; derivative now correctly uses prevPidTemp stored before update for proper rate-of-change calculation
- Rev 57: PRESSURE CALIBRATION FIX - Fixed unit mismatch where ADC counts were compared to voltage values; now converts to voltage before calculations, stores pressureOffset as voltage (not ADC counts), added MIN_CAL_VOLTAGE_RANGE constant (0.5V), improved debug output with voltage values, updated stale EEPROM comment in initPressure()
- Rev 58: PRE-INFUSION PULSE RATE RANGE - Unified validation to 100-1000ms range in both EEPROM load and Web handler, with 500ms default; user configurable via Web UI
- Rev 59: SCALE RELIABILITY TRACKING - Brew always starts regardless of scale status; added scaleReliableEntireShot flag to track scale reliability throughout shot; weight-based stop only if scale reliable for entire shot; time-based stop as fallback if scale ever becomes unreliable; removed blocking scale validation from brew start
- Rev 60: PID INTEGRAL RESET - Reset pid_integral to 0 during setup incomplete to prevent windup accumulation; ensures clean PID response when setup completes
- Rev 61: CLEANUP - Removed duplicate initRelays() call in setup(); relays are initialized once at start
- Rev 62: DISPLAY FIX - Moved firstRunDisplayed flag outside block scope and reset when setupComplete; allows first-run screen to redraw if calibration is reset
- Rev 63: BOOST INDICATOR - Added "BOOST" status indicator on OLED during PID boost phase; displays alongside BREW/FILL/PRE icons on status line
- Rev 64: EMA DOCUMENTATION - Added detailed comments explaining α=0.1 temperature filter coefficient choice; documents trade-offs between noise reduction and responsiveness

PRODUCTION EXCELLENCE ACHIEVED - FULLY NON-BLOCKING ARCHITECTURE

Features:
 - Boiler temperature read via MAX31865 (PT100 3-wire RTD)
 - OLED Display (1.5" 128x128 SH1107 Driver, IIC 4 Pins) with partial updates
 - NAU7802 24-bit ADC Load Cell (I2C interface) for shot weight measurement
 - Single-task architecture: one explicitly-pinned FreeRTOS task (MainTask)
   runs PID control, web UI, display, sensors, and the state machine in a
   single sequential loop. PID control (updatePidControl()) is self-gated
   to 100ms using the same millisElapsed() pattern as every other periodic
   subsystem — no cross-task shared state, no mutexes. (An earlier revision
   split this across two tasks on two cores; collapsed back to one after an
   architecture review found the split didn't deliver the isolation it
   assumed — see project notes.)
 - Comprehensive state machine with error handling and safety margins
 - Real-time Web UI with Chart.js shot graphing
 - Complete EEPROM parameter storage including PID gains and fill probe threshold
 - Parameter validation ensuring pre-infusion/PID boost times ≤ shot target time
 - Hardware safety cutoff and watchdog systems
 - Non-blocking operation in the steady-state tasks; a handful of brief delay()
   calls remain in one-time init and calibration paths (see sensors.cpp,
   calibration code) — not in the control or display loop
 - Momentary brew button with rising edge detection and double-press protection
 - 3-wire pressure transducer (0.5-4.5V, 0-2MPa) with Web UI calibration and pump control
 - Comprehensive EEPROM parameter storage (72 bytes, addresses 0-71, no conflicts)

NOTE: Fully requirements-compliant production firmware per specification document.
*/

// BENCH_MODE is now defined in config.h — it must be visible to every .cpp
// file in the sketch (e.g. relays.cpp's set_boiler_element()), not just this
// one, so the macro itself lives there instead of here.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
// The 6 web-asset PROGMEM headers (bootstrap_custom.h, bootstrap_js.h,
// apexcharts_js.h, web_ui_css.h, web_ui_html.h, web_ui_js.h) are now included
// only in web_api.cpp — see the comment at the top of that file for why
// including them here too would duplicate ~700KB of data in flash.
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_NAU7802.h>
#include <EEPROM.h>
#include <Adafruit_MAX31865.h>
#include <esp_task_wdt.h>
#include <atomic>
#include "config.h"
#include "error_system.h"
#include "shared_state.h"
#include "state_machine.h"
#include "pid_control.h"
#include "relays.h"
#include "sensors.h"
#include "calibration.h"
#include "eeprom_store.h"
#include "display.h"
#include "web_api.h"

// Pin assignments, EEPROM map, and cross-cutting safety constants now live in
// config.h. PID gains, setpoint, boost/pre-infusion flags, allowUnsafePid,
// filtered temp, PID output percent, boiler-heating state, and cached raw
// temp now live in shared_state.h/.cpp (accessed via getters/setters only).
// Error flags and systemMessage now live in error_system.h/.cpp.

// --- Hardware Objects ---
// OLED Display - SH1107 128x128 I2C (Arduino Nano ESP32: A4=SDA/GPIO11, A5=SCL/GPIO12)
U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// NAU7802 Load Cell (I2C interface, shares bus with OLED)
Adafruit_NAU7802 nau;

// MAX31865 PT100 RTD sensor
Adafruit_MAX31865 rtdSensor = Adafruit_MAX31865(PIN_TEMP_CS);

// Control parameters (all temperatures in Fahrenheit)
// setpointTemp, Kp/Ki/Kd now live in shared_state.cpp — use
// getSetpointTemp()/setSetpointTemp(), getKp()/setKp(), etc.
double originalSetpointTemp = 200.0;   // Store original setpoint for predictive compensation

const double tempCompensation = 2.0;  // Fahrenheit, predictive compensation for cold water inrush

// Wi-Fi (stub)
const char* ssid = "espresso_AP";
const char* password = "espresso";
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// The single task running everything (PID control included) — declared here
// (used by setup() below, defined alongside mainTask() near loop() at the
// bottom of the file). One task, one core: there used to be a second task
// (the old "ControlTask") pinned to Core 1 for PID; it's been folded into
// this one (see pid_control.h) since the actual PID computation is a few
// microseconds of work every 100ms and never justified a dedicated core —
// see the architecture analysis this session for why.
static TaskHandle_t mainTaskHandle = NULL;

// Web server rate limiting
unsigned long lastWebRequest = 0;
const unsigned long WEB_RATE_LIMIT = 50;  // 50ms between requests

// WebSocket variables
unsigned long lastWebSocketBroadcast = 0;
const unsigned long WEBSOCKET_BROADCAST_INTERVAL = 100;  // 100ms WebSocket updates

// EEPROM signature, size, and ADDR_* memory map now live in config.h.

// --- State Machine ---
// SystemState enum now lives in state_machine.h (shared with pid_control.cpp).
// No longer volatile — single task, no concurrent reader/writer to guard
// visibility against.
SystemState currentState = STATE_BOOT;
SystemState previousState = STATE_BOOT;

// First-run setup state
bool setupComplete = false;        // True when required calibrations are done
bool tempCalComplete = false;      // Temperature probe calibration complete (REQUIRED for PID)
bool pressureCalComplete = false;  // Pressure transducer calibration complete (REQUIRED for PID)
bool boilerFilled = false;         // Steam boiler has been filled (tracked independently, not required for PID)
bool scaleCalComplete = false;     // Scale calibration complete (optional - system uses shot time target if scale unavailable)
bool fillCalComplete = false;      // Fill probe calibration complete (optional, non-blocking)
// allowUnsafePid now lives in shared_state.cpp (getAllowUnsafePid()/setAllowUnsafePid()).

// Shot parameters and targets
double shotTargetWeight = 36.0;   // grams, default target weight
double shotTargetTime = 30.0;     // seconds, default target time
unsigned long brewStartTime = 0;  // milliseconds when brew started

// PID Boost and Pre-infusion parameters
// pidBoostActive and preInfusionPhase now live in shared_state.cpp
// (getPidBoostActive()/setPidBoostActive(), getPreInfusionPhase()/setPreInfusionPhase()).
double pidBoostDuration = 4.0;      // seconds, default PID boost duration
double preInfusionDuration = 8.0;   // seconds, default pre-infusion duration
double preInfusionPulseRate = 500;  // milliseconds, configurable pulse on/off duration
unsigned long pidBoostStartTime = 0;
unsigned long preInfusionStartTime = 0;

// currentTemp and cachedRawTemp now live in shared_state.cpp
// (getTemp()/setTemp(), getCachedRawTemp()/setCachedRawTemp()).
// pidOutput is now a local variable inside pid_control.cpp's pid_step().

// Temperature rate-of-change protection (unified limit for sensor and PID)
// MAX_TEMP_RATE now lives in config.h; lastTemp/lastTempTime are now private
// to sensors.cpp's read_boiler_temp().

// Display variables
double shotTime = 0.0;
double shotWeight = 0.0;
bool scaleConnected = false;
float scaleValidationWeight = 0.0;           // Last weight for stability validation
unsigned long scaleValidationStartTime = 0;  // When validation started
int scaleValidationCount = 0;                // Consecutive stable readings
// boilerHeating and preInfusionActive now live in shared_state.cpp
// (getBoilerHeatingState()/setBoilerHeatingState(), getPreInfusionActive()/
// setPreInfusionActive()). systemMessage and the error-flag bus now live in
// error_system.h/.cpp.
bool brewActive = false;
bool fillActive = false;

// Flow rate calculation variables
double currentFlowRate = 0.0;  // ml/s extraction rate
double lastWeightForFlow = 0.0;
unsigned long lastFlowCalculation = 0;
// FLOW_CALCULATION_INTERVAL now lives in config.h (sensors.cpp needs it too).
double smoothedFlowRate = 0.0;                        // Smoothed flow rate for display
bool flowRateFirstCalculation = true;                 // Reset flag for new brew cycles

// Shot ratio tracking variables
double doseWeight = 18.0;       // Default dose weight in grams (user configurable)
double currentShotRatio = 0.0;  // Current output÷dose ratio (e.g., 2.0 = 1:2 ratio)

// NAU7802 scale variables
int32_t scaleZeroOffset = 0;  // Zero offset for tare functionality

// Scale reading stability variables are now local statics inside
// sensors.cpp's readScale() — nothing else referenced them.

// Scale reliability tracking for weight-based brew stop
// If scale becomes unreliable at ANY point during shot, brew stops by time instead of weight
bool scaleReliableEntireShot = true;  // Reset to true at brew start, set false if scale fails

// PID output tracking: currentPIDOutputPercent now lives in shared_state.cpp.

// Cleaning cycle variables
bool cleaningActive = false;
int cleaningCycle = 0;
unsigned long cleaningStartTime = 0;
unsigned long cleaningPumpOnTime = 0;
unsigned long cleaningPumpOffTime = 0;
bool cleaningPumpState = false;
int pumpFailureCount = 0;
unsigned long completionMessageTime = 0;
bool pumpTimeoutTriggered = false;
bool completionMessageActive = false;

// --- Performance Monitoring Variables ---
// PID timing validation: written by pid_control.cpp's checkPIDTiming(), read
// by logPerformanceMetrics() here. Not static, purely so pid_control.cpp can
// `extern` them — a cross-file linkage need, not a synchronization one, so
// no volatile either (single task, nothing reads these asynchronously).
// PID_JITTER_THRESHOLD lives in config.h for the same linkage reason.
unsigned long lastPIDTime = 0;
unsigned long maxPIDJitter = 0;
unsigned long pidJitterCount = 0;

// Memory monitoring
static unsigned long lastMemoryCheck = 0;
static size_t minFreeHeap = SIZE_MAX;
const size_t MEMORY_WARNING_THRESHOLD = 40000;     // Alert if free heap < 40KB
const unsigned long MEMORY_CHECK_INTERVAL = 5000;  // Check every 5 seconds

// Performance metrics — not static: written from web_api.cpp's
// handleStatus()/broadcastWebSocketData(), read by logPerformanceMetrics()
// here (same cross-file-linkage reason as the PID jitter counters above).
unsigned long webRequestCount = 0;
unsigned long webSocketMessageCount = 0;
unsigned long maxWebResponseTime = 0;
unsigned long totalWebResponseTime = 0;

// Load monitoring flags — not static for the same cross-file-linkage reason.
bool performanceMonitoringEnabled = true;  // Read/written from serial commands and pid_control.cpp
static bool memoryWarningActive = false;
bool pidTimingWarningActive = false;  // Written by pid_control.cpp, read here

// PID logging flags (set by updatePidControl(), processed by processPIDTaskLogs())
bool pidLogTempRateError = false;
double pidLogTempRate = 0.0;
bool pidLogTimingWarning = false;
unsigned long pidLogJitter = 0;
unsigned long pidLogJitterCount = 0;
bool pidLogTimingNormalized = false;

// Scale variables
float scaleCalibrationFactor = 1.0;
bool scaleInitialized = false;
unsigned long lastScaleRead = 0;
const unsigned long SCALE_READ_INTERVAL_IDLE = 100;  // 100ms scale reading during idle
const unsigned long SCALE_READ_INTERVAL_BREW = 50;   // 50ms scale reading during brew for high precision

// Cleaning cycle constants
int cleaningCycleCount = 10;                              // Number of cleaning cycles (configurable)
const unsigned long CLEANING_PUMP_ON_DURATION = 10000;   // 10 seconds pump ON
const unsigned long CLEANING_PUMP_OFF_DURATION = 10000;  // 10 seconds pump OFF
const unsigned long CLEANING_MAX_DURATION = 300000;      // 5 minutes maximum cleaning time
const unsigned long PUMP_RESPONSE_TIMEOUT = 15000;       // 15 seconds pump response timeout
const int PUMP_FAILURE_COUNT = 3;                        // Consecutive failures before stop

// scaleMux removed — every reader/writer of shotWeight/scale calibration
// readings is on the single task; it was never actually guarding against a
// second task even before today's change (confirmed in the architecture
// review this session).

// CalStep enum, calStep global, and isCalibrating()/isPressureCal()/
// isFillScaleCal() now live in calibration.h (shared broadly).
int fillProbeDryReading = 0;
int fillProbeWetReading = 0;
int fillProbeThreshold = 512;  // Midpoint default
int32_t scaleZeroReading = 0;
int32_t scaleKnownWeightReading = 0;

// Fill system variables
bool fillProbeWet = false;
unsigned long lastFillCheck = 0;
const unsigned long FILL_CHECK_INTERVAL = 1000;  // 1s fill probe check
const unsigned long FILL_PULSE_DURATION = 2000;  // 2s fill pulse duration
// FILL_MAX_DURATION/STARTUP_FILL_TIMEOUT now live in config.h.
unsigned long fillStartTime = 0;  // Reset every 2s pulse cycle (pump on/off timing)
bool fillInProgress = false;

// Separate from fillStartTime on purpose: fillStartTime resets every 2s pulse
// cycle, so a timeout check against it would never actually elapse during a
// prolonged dry fill. fillAttemptStartTime marks when THIS fill attempt began
// and is never reset until the attempt ends — this is what the safety timeout
// below actually measures against.
unsigned long fillAttemptStartTime = 0;
unsigned long currentFillTimeoutMs = FILL_MAX_DURATION;

// True once at boot if boilerFilled was already true in EEPROM (i.e. this
// isn't a fresh install) — gives the very first fill attempt after boot the
// longer STARTUP_FILL_TIMEOUT grace period, exactly like the old blocking
// runStartupFillCycle() did, before being consumed (set false) by that first
// startFillRoutine() call. All later fills use the normal FILL_MAX_DURATION.
bool pendingStartupFillGrace = false;

// Brew switch variables
bool brewSwitchPressed = false;
bool prevBrewSwitchState = false;
unsigned long lastBrewSwitchDebounce = 0;
const unsigned long BREW_SWITCH_DEBOUNCE = 50;   // 50ms debounce
bool lastBrewButtonState = false;                // For rising edge detection
unsigned long lastBrewButtonPress = 0;           // For double-press protection
const unsigned long BREW_BUTTON_COOLDOWN = 200;  // 200ms between button presses

// Pressure measurement variables
// PIN_PRESSURE now lives in config.h.
double currentPressure = 0.0;  // Current pressure in Bar
double pressureOffset = 0.0;   // Zero pressure calibration offset
double pressureScale = 1.0;    // Pressure scaling factor
bool pressureLockoutActive = false;  // Pressure safety lockout state (prevents pump oscillation)
// isPressureCal() derived from calStep (CAL_PRESSURE_ZERO or CAL_PRESSURE_HIGH)
int pressureZeroReading = 0;
int pressureHighReading = 0;
double pressureHighValue = 8.0;  // User-configurable high pressure calibration point (default 8 BAR)
unsigned long lastPressureRead = 0;
const unsigned long PRESSURE_READ_INTERVAL = 250;  // 250ms pressure reading

// Pressure smoothing (exponential moving average): filteredPressure,
// pressureAlpha, and pressureFirstReading are now private to sensors.cpp's
// readPressure() — nothing else referenced them.

// ADC_MAX_VOLTAGE/ADC_RESOLUTION now live in config.h.
// pid_integral/pid_prev_error now live in pid_control.cpp as plain statics —
// no task handle, no mutex; there's only one task, so nothing can touch
// them concurrently.

// EEPROM save re-entrancy guard is a static bool inside saveParametersToEEPROM()
// Deferred EEPROM save — reduces flash wear by coalescing rapid parameter changes.
// Not volatile — single task; this also fixes a latent mismatch where
// calibration.cpp/web_api.cpp's `extern bool eepromDirty` declarations never
// had the qualifier this definition did.
bool eepromDirty = false;

// stateMux removed — single task, changeState()/getState() no longer need
// a critical section. controlMux/tempMux/cachedTempMux were already folded
// into shared_state.cpp and have likewise had their locks removed there.

// FreeRTOS variables removed - using simple millis() timing per requirements

// Unified display timing (matches Web UI for perfect sync)
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_IDLE = 100;   // 100ms unified rate
const unsigned long DISPLAY_UPDATE_BREW = 100;   // 100ms unified rate
const unsigned long DISPLAY_UPDATE_ERROR = 100;  // 100ms unified rate

// Error handling and watchdog
unsigned long lastLoopTime = 0;
unsigned long lastWatchdogCheck = 0;
// LOOP_TIMEOUT now lives in config.h.
const unsigned long LOOP_CHECK_INTERVAL = 1000;  // Check every second
// loopTimeoutTriggered removed — it gated two recovery branches for
// ERR_FLAG_LOOP_TIMEOUT, which nothing ever set (see handleSystemErrors()).
// rtdFault is now private to sensors.cpp (initRTDSensor()/read_boiler_temp()).

// Initialization failure tracking
bool displayFailed = false;
bool i2cFailed = false;
bool webServerFailed = false;

// PID error monitoring with startup exception
bool setpointAchieved = false;
unsigned long setpointAchievementTime = 0;
const unsigned long SETPOINT_ACHIEVEMENT_DELAY = 30000;  // 30 seconds after startup
const double PID_ERROR_THRESHOLD = 15.0;                 // 15°F deviation threshold
const unsigned long PID_ERROR_DURATION = 30000;          // 30 seconds duration for PID error
unsigned long pidErrorStartTime = 0;
bool pidErrorActive = false;

// System health monitoring
unsigned long lastHealthCheck = 0;
const unsigned long HEALTH_CHECK_INTERVAL = 30000;  // 30 seconds
unsigned long totalUptime = 0;
unsigned long systemStartTime = 0;

// Hardware watchdog timeout (using the unsigned long version defined above)

// --- Forward Declarations ---
// Error management
bool hasError();
bool isCriticalError();
bool isRecoverableError();
bool isDisplayOnlyError();
bool requiresManualReboot();
void emergencyStop();
void recomputeSetupComplete();
bool isPidAllowed();
double getSetpointTemp();
bool getPidBoostActive();
bool getPreInfusionPhase();
bool getPreInfusionActive();
bool getAllowUnsafePid();
void setAllowUnsafePid(bool allowed);
void setPidBoostActive(bool active);
void setSetpointTemp(double temp);
void addSetpointTemp(double delta);
double getCurrentPIDOutputPercent();
void setCurrentPIDOutputPercent(double percent);

// State management
void changeState(SystemState newState);
SystemState getState();
const char* getStateName(SystemState state);

// Relay control now declared in relays.h (bool-returning).

// Calibration now declared in calibration.h.
// EEPROM persistence now declared in eeprom_store.h.

// Routines
void startBrewRoutine();
void stopBrewRoutine();
void startFillRoutine();
void stopCleaningCycle();

// PID control now declared in pid_control.h (pid_step() is private to
// pid_control.cpp; only updatePidControl()/resetPidIntegral() are public).

// The single task running everything, explicitly pinned in setup(); see its
// definition near loop() for why Arduino's own loop() is now an inert stub.
void mainTask(void* parameter);

// Sensors now declared in sensors.h.
// Error system (setError/clearError/getErrorFlags/etc.) declared in error_system.h.

// Hardware init (initRelays() now declared in relays.h; runStartupFillCycle()
// removed — see updateFillRoutine()/setup() for the non-blocking replacement)

// Display now declared in display.h.

// System operations
void startCleaningCycle();
void checkFillLevel();
void updateFillRoutine();
void checkBrewSwitch();
void checkSystemHealth();
void checkWatchdog();
void handleSystemErrors();
void checkMemoryUsage();
void logPerformanceMetrics();

// Web handlers, WebSocket, and buildStatusJSON() now declared in web_api.h.
void processPIDTaskLogs();

// --- Timing Helper Functions ---
bool millisElapsed(unsigned long startTime, unsigned long interval) {
  // Handles millis() rollover correctly (every ~49 days)
  return (millis() - startTime) >= interval;
}

// --- Setup & Control Helpers ---
void recomputeSetupComplete() {
  setupComplete = (tempCalComplete && pressureCalComplete);
}

// getAllowUnsafePid/setAllowUnsafePid, getSetpointTemp/setSetpointTemp/
// addSetpointTemp, getPidBoostActive/setPidBoostActive, getPreInfusionPhase/
// getPreInfusionActive, getCurrentPIDOutputPercent/setCurrentPIDOutputPercent
// now live in shared_state.cpp.

bool isPidAllowed() {
  return setupComplete || getAllowUnsafePid();
}

// initScale()/readScale()/tareScale()/calculateFlowRate()/calculateShotRatio()
// now live in sensors.cpp. calibrateScale(float) was removed (zero callers).

// startFillProbeCalibration()/startScaleCalibration()/processCalibrationStep()/
// completeScaleCalibration() now live in calibration.cpp.

// initDisplay()/updateDisplay() now live in display.cpp.
// jsonStatusBuffer/webSocketBuffer now live in web_api.cpp. The unused
// httpResponseBuffer and displayBuffer (zero references — dead code, same
// class of issue as the removed set_ssr()/calibrateScale()) were removed.

// celsiusToFahrenheit()/fahrenheitToCelsius()/initRTDSensor()/
// read_boiler_temp() now live in sensors.cpp.

// --- Error Management Functions ---
void emergencyStop() {
  // Stop heating and pumping systems for critical errors (per requirements).
  // No critical section needed — single task, nothing can interleave here.
  set_pump(false);
  set_brew_solenoid(false);
  set_fill_solenoid(false);
  set_boiler_element(false);
  brewActive = false;
  fillActive = false;
  fillInProgress = false;

  // Stop control variables
  setPidBoostActive(false);
  setPreInfusionActive(false);
  setPreInfusionPhase(false);

  // Critical errors require ERROR state and manual reboot
  if (requiresManualReboot()) {
    changeState(STATE_ERROR);
    Serial.println("CRITICAL ERROR: Heating and pumping systems stopped - manual reboot required");
  } else {
    // Non-critical errors maintain operation
    Serial.println("Non-critical error: System continues operation with error display");
  }
}

// updateSystemMessage()/setError()/clearError()/hasError()/isRecoverableError()/
// isCriticalError()/isDisplayOnlyError() live in error_system.cpp (declared
// in error_system.h, already included above). This block used to duplicate
// those definitions here too — a leftover from the file-decomposition pass
// that would have failed to link (multiple definition of 7 symbols) on any
// actual build attempt. Removed as part of this reliability pass.

bool requiresManualReboot() {
  // During initial commissioning (setupComplete=false), demote all errors to
  // recoverable/display-only so transient sensor faults on first hardware
  // connection never block the calibration workflow (which requires STATE_IDLE).
  // PID is already gated off by isPidAllowed() during this phase regardless.
  if (!setupComplete) return false;

  // Normal operation: RTD (sensor fault) and PID (control failure) require manual reboot.
  // TEMP (over-temperature) auto-recovers when temperature drops 10°F below threshold.
  uint8_t flags = getErrorFlags();
  return (flags & (ERR_FLAG_RTD | ERR_FLAG_PID)) != 0;
}

void checkSystemHealth() {
  unsigned long now = millis();

  // Update total uptime
  totalUptime = now - systemStartTime;

  // Check system health periodically
  if (now - lastHealthCheck >= HEALTH_CHECK_INTERVAL) {
    lastHealthCheck = now;

    // Log system health status
  double temp = getTemp();
  Serial.printf("System Health: Uptime=%lu ms, FreeHeap=%lu bytes, Temp=%.1f°F, State=%s\n",
                  totalUptime, ESP.getFreeHeap(), temp, getStateName(currentState));

    // Check memory fragmentation
    if (ESP.getFreeHeap() < 10000) {  // Less than 10KB free
      Serial.println("WARNING: Low memory detected");
    }
  }
}

void checkWatchdog() {
  unsigned long now = millis();

  // Update loop timing monitoring
  lastLoopTime = now;

  // Check loop timing periodically
  if (now - lastWatchdogCheck >= LOOP_CHECK_INTERVAL) {
    lastWatchdogCheck = now;

    // Note: Hardware watchdog (esp_task_wdt) handles actual timeout detection
    // This function monitors loop health and updates timing metrics
    // The hardware watchdog will trigger panic if loop() doesn't call esp_task_wdt_reset()
  }
}

void checkPIDError() {
  // Only check PID error after setpoint has been achieved and system is stable
  if (!setpointAchieved) {
    // Check if setpoint has been achieved for the required duration
    double temp = getTemp();
    double setpoint = getSetpointTemp();
    if (abs(temp - setpoint) < 2.0) {  // Within 2°F of setpoint
      if (setpointAchievementTime == 0) {
        setpointAchievementTime = millis();
      } else if (millisElapsed(setpointAchievementTime, SETPOINT_ACHIEVEMENT_DELAY)) {
        setpointAchieved = true;
        Serial.println("PID monitoring enabled - setpoint achieved and stable");
      }
    } else {
      // Reset achievement timer if temperature deviates
      setpointAchievementTime = 0;
    }
    return;
  }

  // PID error monitoring (only after setpoint achievement)
  double tempDeviation = abs(getTemp() - getSetpointTemp());

  if (tempDeviation > PID_ERROR_THRESHOLD) {
    if (!pidErrorActive) {
      pidErrorActive = true;
      pidErrorStartTime = millis();
      Serial.printf("PID Error detected: %.1f°F deviation from setpoint\n", tempDeviation);
    } else if (millisElapsed(pidErrorStartTime, PID_ERROR_DURATION)) {
      // PID error sustained for required duration - trigger critical error
      setError(ERR_FLAG_PID);
      Serial.println("CRITICAL PID ERROR: Temperature deviation sustained - emergency stop");
    }
  } else {
    // Temperature back within acceptable range
    if (pidErrorActive) {
      pidErrorActive = false;
      Serial.println("PID Error cleared - temperature back within acceptable range");
    }
  }
}

void handleSystemErrors() {
  // Check for temperature safety with auto-recovery
  double temp = getTemp();
  if (temp >= safetyTemp) {
    setError(ERR_FLAG_TEMP);
    Serial.printf("OVER-TEMP: %.1f°F exceeds safety limit of %.1f°F - PID disabled\n", temp, safetyTemp);
  } else if (hasErrorFlag(ERR_FLAG_TEMP) && temp < (safetyTemp - 10.0)) {
    // Temperature must drop 10°F below safety limit to auto-recover (hysteresis)
    // This prevents rapid on/off cycling near the safety threshold
    clearError(ERR_FLAG_TEMP);
    Serial.printf("OVER-TEMP AUTO-RECOVERY: %.1f°F now below threshold - PID resumed\n", temp);
    Serial.println("Note: Consider recalibrating temperature probe if this occurs frequently");
  }

  // Handle scale errors (auto-recovery when scale reconnects)
  if (hasErrorFlag(ERR_FLAG_SCALE) && scaleConnected) {
    clearError(ERR_FLAG_SCALE);
  }

  // Handle pressure errors (managed by pressure reading function with hysteresis)
  // Pressure recovery is handled automatically in readPressure() function

  // ERR_FLAG_LOOP_TIMEOUT's recovery handling used to live here. Removed:
  // nothing in this codebase ever sets that flag (confirmed by grep — it was
  // vestigial from an earlier software-watchdog design), so this branch
  // could never execute. The hardware watchdog (esp_task_wdt) is what
  // actually catches a stalled loop, and it hard-resets rather than
  // recovering gracefully — this dead branch implied a capability that
  // didn't exist.

  // Check PID error conditions
  checkPIDError();

  // PID bypass warning for incomplete setup (non-error informational message)
  if (!setupComplete && getAllowUnsafePid() && !hasError() && !completionMessageActive) {
    if (systemMessage.length() == 0 || systemMessage == "PID BYPASS") {
      systemMessage = "PID BYPASS";
    }
  } else if (systemMessage == "PID BYPASS") {
    systemMessage = "";
  }
}

// initRelays()/set_brew_solenoid()/set_pump()/set_fill_solenoid()/
// set_boiler_element() now live in relays.cpp (bool-returning). The dead
// set_ssr() "legacy" wrapper (zero callers) has been removed.

// readFillProbe()/readFillProbeRaw()/initFillSystem() now live in sensors.cpp.

// runStartupFillCycle() removed — startup fill is now handled non-blocking
// by the normal FILL state machine (startFillRoutine()/updateFillRoutine()),
// with pendingStartupFillGrace giving the first post-boot attempt the same
// 120s timeout this blocking version used to. See setup() and
// updateFillRoutine() for the rest of this change.

// --- Brew Switch Functions ---
bool readBrewSwitch() {
  // Read momentary brew switch (NO contact to GND)
  // Momentary Logic: Press to start brew (if IDLE), press again to stop brew (if BREWING)
  // Expecting LOW when switch is pressed (connected to GND)
  return digitalRead(PIN_BREW_SWITCH) == LOW;
}

void initBrewSwitch() {
  pinMode(PIN_BREW_SWITCH, INPUT_PULLUP);
  prevBrewSwitchState = readBrewSwitch();
  brewSwitchPressed = prevBrewSwitchState;
  lastBrewButtonState = brewSwitchPressed;  // Initialize rising edge detection
  Serial.printf("Brew switch initialized. State: %s\n", brewSwitchPressed ? "PRESSED" : "RELEASED");
}

void checkBrewSwitch() {
  bool currentSwitchState = readBrewSwitch();

  // Debounce the switch
  if (currentSwitchState != prevBrewSwitchState) {
    lastBrewSwitchDebounce = millis();
  }

  // Check for stable switch state after debounce
  if (millisElapsed(lastBrewSwitchDebounce, BREW_SWITCH_DEBOUNCE)) {
    if (currentSwitchState != brewSwitchPressed) {
      brewSwitchPressed = currentSwitchState;
      Serial.printf("Brew switch %s\n", brewSwitchPressed ? "PRESSED" : "RELEASED");

      // Rising edge detection for momentary button logic
      if (brewSwitchPressed && !lastBrewButtonState) {
        // Button pressed (rising edge) - check double-press protection
        if (millisElapsed(lastBrewButtonPress, BREW_BUTTON_COOLDOWN)) {
          lastBrewButtonPress = millis();

          // Check for brew start or stop
          if (currentState == STATE_IDLE && !isCriticalError() && !isPressureCal() && !cleaningActive) {
            // Start brew if in IDLE state and no blocking conditions
            Serial.println("Momentary brew button: Starting brew");
            changeState(STATE_BREW_START);
          } else if (currentState == STATE_BREW_ACTIVE && !isCriticalError()) {
            // Stop brew if currently brewing
            Serial.println("Momentary brew button: Stopping brew");
            changeState(STATE_BREW_END);
          } else if (currentState == STATE_CLEANING) {
            // Stop cleaning cycle if currently cleaning (mechanical backup)
            Serial.println("Momentary brew button: Stopping cleaning cycle");
            stopCleaningCycle();
            changeState(STATE_IDLE);
          } else if (isPressureCal()) {
            Serial.println("Brew button ignored - pressure calibration in progress");
          } else if (isCriticalError()) {
            Serial.printf("Brew button ignored - critical error active: %s\n", systemMessage.c_str());
          } else if (currentState == STATE_ERROR) {
            Serial.printf("Brew button logged during ERROR state: %s\n", systemMessage.c_str());
          } else {
            Serial.printf("Brew button ignored - invalid state: %s\n", getStateName(currentState));
          }
        } else {
          Serial.println("Brew button ignored - double-press protection active");
        }
      }

      // Update rising edge detection state
      lastBrewButtonState = brewSwitchPressed;
    }
  }

  prevBrewSwitchState = currentSwitchState;
}

// --- Brew Routine Functions ---
// --- State Machine Functions ---
void changeState(SystemState newState) {
  if (currentState != newState) {
    previousState = currentState;
    currentState = newState;
    Serial.printf("State change: %d -> %d\n", previousState, currentState);
    // Deliberately no EEPROM flush here. This used to synchronously call
    // saveParametersToEEPROM() on every transition into STATE_IDLE with
    // dirty parameters — an EEPROM.commit() can take tens of milliseconds
    // on this platform, and calling it from inside the state-machine
    // transition function blocked the entire control loop for that long on
    // every brew-to-idle transition. That's exactly the hazard the
    // deferred-EEPROM design (eepromDirty + the 2-second periodic flush in
    // handleSystemMonitoring()) exists to avoid; this eager path bypassed
    // it. The periodic flush already catches the same dirty parameters
    // within 2 seconds, without blocking a state transition to do it.
  }
}

// Single-task now — this is just a named accessor for currentState (kept so
// every existing call site stays unchanged), not a synchronization point.
SystemState getState() {
  return currentState;
}

// getTemp/setTemp and getCachedRawTemp/setCachedRawTemp now live in shared_state.cpp.

const char* getStateName(SystemState state) {
  static char cleaningStateBuffer[20];

  switch (state) {
    case STATE_BOOT: return "BOOTING";
    case STATE_IDLE: return "READY";
    case STATE_BREW_START: return "BREW START";
    case STATE_BREW_ACTIVE: return "BREWING";
    case STATE_BREW_END: return "BREW END";
    case STATE_FILL: return "FILL";
    case STATE_CLEANING:
      if (cleaningActive) {
        snprintf(cleaningStateBuffer, sizeof(cleaningStateBuffer), "CLEANING #%d", cleaningCycle);
        return cleaningStateBuffer;
      } else {
        return "CLEANING";
      }
    case STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

void updateStateMachine() {
  switch (currentState) {
    case STATE_BOOT:
      // Boot initialization complete, move to idle
      changeState(STATE_IDLE);
      break;

    case STATE_IDLE:
      // Check for critical error conditions - stop heating/pumping but maintain operation
      if (isCriticalError()) {
        emergencyStop();  // Stop heating/pumping systems; may itself transition to
                           // STATE_ERROR if this fault requires manual reboot (RTD/PID)
        break;
      }

      // Check for fill needed (only if no errors, and not mid pressure calibration —
      // checkFillLevel() won't actually start the fill hardware during calibration,
      // so flipping the state label here without that guard left the system stuck
      // in STATE_FILL with no way back to IDLE once calibration finished).
      if (!hasError() && !fillProbeWet && !fillInProgress && !isPressureCal()) {
        changeState(STATE_FILL);
        break;
      }

      // Brew starting is now handled by momentary button logic in checkBrewSwitch()
      break;

    case STATE_BREW_START:
      // Re-check for a critical error here (not just at the button press that
      // requested this state) — closes the gap between checkBrewSwitch()'s check
      // and this state actually being processed.
      if (isCriticalError()) {
        Serial.println("Critical error detected before brew start - aborting");
        changeState(STATE_IDLE);
        break;
      }
      // Start brew routine
      startBrewRoutine();
      changeState(STATE_BREW_ACTIVE);
      break;

    case STATE_BREW_ACTIVE:
      // Check for brew stop conditions (priority order per requirements)

      // 1. Critical error during brew - stop heating/pumping but maintain operation
      if (isCriticalError()) {
        Serial.printf("Critical error during brew: %s - stopping heating/pumping\n", systemMessage.c_str());
        emergencyStop();  // Stop heating/pumping systems; transitions to STATE_ERROR
                           // itself if this fault requires manual reboot (RTD/PID) —
                           // only fall through to BREW_END for faults that don't, so
                           // that transition doesn't immediately overwrite ERROR.
        if (!requiresManualReboot()) {
          changeState(STATE_BREW_END);  // End brew but maintain system operation
        }
        break;
      }

      // 2. Weight-based stop: ONLY if scale was reliable for ENTIRE shot
      // Scale must be connected AND have provided reliable readings throughout
      if (scaleReliableEntireShot && scaleConnected && shotWeight >= shotTargetWeight) {
        Serial.printf("Target weight reached: %.1fg (scale reliable entire shot)\n", shotWeight);
        changeState(STATE_BREW_END);
        break;
      }

      // 3. Time-based stop (fallback): If scale ever became unreliable OR not connected
      if ((!scaleReliableEntireShot || !scaleConnected) && shotTime >= shotTargetTime) {
        if (!scaleReliableEntireShot) {
          Serial.printf("Target time reached: %.1fs (scale became unreliable during shot)\n", shotTime);
        } else {
          Serial.printf("Target time reached: %.1fs (no scale)\n", shotTime);
        }
        changeState(STATE_BREW_END);
        break;
      }

      // 5. Max Timeout: 60s (absolute safety limit)
      if (shotTime >= maxBrewTime) {
        Serial.printf("Max brew timeout reached: %.1fs\n", shotTime);
        changeState(STATE_BREW_END);
        break;
      }

      // 6. Manual stop is now handled by momentary button logic in checkBrewSwitch()
      break;

    case STATE_BREW_END:
      // Stop brew routine
      stopBrewRoutine();
      changeState(STATE_IDLE);
      break;

    case STATE_FILL:
      // Fill routine active
      if (fillProbeWet || isCriticalError()) {
        fillActive = false;
        fillInProgress = false;
        if (isCriticalError()) {
          emergencyStop();  // Stop heating/pumping systems
          // DO NOT change state - maintain operation for non-critical functions
        } else {
          changeState(STATE_IDLE);
        }
      }
      break;

    case STATE_CLEANING:
      // Critical error - abort cleaning immediately, matching every other active
      // state. Without this, a TEMP fault (which deliberately doesn't change
      // currentState, to allow hysteresis auto-recovery) left this cycling logic
      // running unaware, free to turn the pump back on moments after
      // emergencyStop() turned it off — fighting pid_control.cpp's own
      // independent TEMP-triggered pump cutoff.
      if (isCriticalError()) {
        Serial.printf("Critical error during cleaning: %s - stopping cleaning\n", systemMessage.c_str());
        stopCleaningCycle();
        emergencyStop();  // Stops pump/solenoids again (idempotent) and transitions
                           // to STATE_ERROR itself if this fault requires manual reboot.
        if (!requiresManualReboot()) {
          changeState(STATE_IDLE);
        }
        break;
      }

      // Cleaning cycle active - pump cycling logic
      if (cleaningActive) {
        // Check maximum cleaning duration safety limit
        if (millisElapsed(cleaningStartTime, CLEANING_MAX_DURATION)) {
          Serial.println("Maximum cleaning duration reached - stopping cleaning");
          stopCleaningCycle();
          changeState(STATE_IDLE);
          break;
        }

        if (cleaningPumpState) {
          // Pump and brew solenoid are ON - check timeout and normal duration
          if (!pumpTimeoutTriggered && millisElapsed(cleaningPumpOnTime, PUMP_RESPONSE_TIMEOUT)) {
            // Pump timeout - stop pump and solenoid, increment failure count
            set_pump(false);
            set_brew_solenoid(false);
            cleaningPumpState = false;
            pumpFailureCount++;
            pumpTimeoutTriggered = true;
            Serial.printf("Pump timeout detected - failure count: %d\n", pumpFailureCount);

            if (pumpFailureCount >= PUMP_FAILURE_COUNT) {
              Serial.println("Pump failure limit reached - stopping cleaning");
              stopCleaningCycle();
              changeState(STATE_IDLE);
              return;
            } else {
              // Continue to next cycle after timeout
              cleaningPumpOffTime = millis();
            }
          }

          if (millisElapsed(cleaningPumpOnTime, CLEANING_PUMP_ON_DURATION)) {
            set_pump(false);
            set_brew_solenoid(false);
            cleaningPumpState = false;
            cleaningPumpOffTime = millis();
            pumpFailureCount = 0;  // Reset failure count on successful cycle
          }
        } else {
          // Pump and brew solenoid are OFF - check if time to turn ON for next cycle
          if (millisElapsed(cleaningPumpOffTime, CLEANING_PUMP_OFF_DURATION)) {
            cleaningCycle++;
            if (cleaningCycle <= cleaningCycleCount) {
              set_brew_solenoid(true);
              set_pump(true);
              cleaningPumpState = true;
              cleaningPumpOnTime = millis();
              pumpTimeoutTriggered = false;
            } else {
              // All cycles complete
              Serial.printf("Cleaning cycle completed successfully - %d cycles finished\n", cleaningCycleCount);
              systemMessage = "CLEANING COMPLETE";
              completionMessageTime = millis();
              completionMessageActive = true;
              stopCleaningCycle();
              changeState(STATE_IDLE);
            }
          }
        }
      } else {
        // Cleaning stopped - return to idle
        changeState(STATE_IDLE);
      }
      break;

    case STATE_ERROR:
      // ERROR state for critical errors
      // TEMP errors auto-recover when temperature drops below threshold
      // RTD/PID errors require manual reboot

      // Auto-recovery for recoverable errors
      if (isRecoverableError()) {
        if (hasErrorFlag(ERR_FLAG_SCALE) && scaleConnected) {
          clearError(ERR_FLAG_SCALE);
          changeState(STATE_IDLE);
          Serial.println("SCALE error auto-recovered");
        }
      }

      // Check if error was cleared (e.g., TEMP auto-recovery in handleSystemErrors)
      if (!hasError()) {
        changeState(STATE_IDLE);
        Serial.println("Error cleared - returning to IDLE state");
      } else if (requiresManualReboot()) {
        // RTD/PID errors require manual reboot
        static unsigned long lastRebootMsg = 0;
        if (millis() - lastRebootMsg > 10000) {
          lastRebootMsg = millis();
          Serial.printf("System in ERROR state - %s error requires manual reboot\n", systemMessage.c_str());
        }
      }
      break;
  }
}

void startBrewRoutine() {
  if (currentState != STATE_BREW_START && currentState != STATE_BREW_ACTIVE) return;

  // Prevent brew during cleaning cycle
  if (cleaningActive) {
    Serial.println("Cannot start brew - cleaning cycle active");
    changeState(STATE_IDLE);
    return;
  }

  // Brew ALWAYS starts regardless of scale status
  // Scale reliability is tracked throughout shot to determine weight vs time stop
  // If scale connected and working: stop by weight target
  // If scale fails at any point during shot: stop by time target
  
  // Initialize scale reliability tracking - assume reliable until proven otherwise
  scaleReliableEntireShot = scaleConnected;  // Start as connected state
  if (scaleConnected) {
    Serial.println("Scale connected - brew will stop by weight target");
  } else {
    Serial.println("Scale not connected - brew will stop by time target");
  }

  brewActive = true;
  shotTime = 0.0;  // Reset shot timer for new brew
  brewStartTime = millis();

  // Reset flow rate calculation variables for new brew
  currentFlowRate = 0.0;
  lastWeightForFlow = shotWeight;
  lastFlowCalculation = millis();
  smoothedFlowRate = 0.0;
  flowRateFirstCalculation = true;

  // Reset shot ratio for new brew
  currentShotRatio = 0.0;

  // Predictive temperature compensation for cold water inrush
  originalSetpointTemp = getSetpointTemp();
  addSetpointTemp(tempCompensation);  // Temporarily increase setpoint
  Serial.printf("Applying temperature compensation: %.1f°F -> %.1f°F\n",
                originalSetpointTemp, getSetpointTemp());

  // Initialize PID Boost and Pre-infusion phases
  setPidBoostActive(true);
  pidBoostStartTime = millis();
  setPreInfusionPhase(true);
  setPreInfusionActive(true);
  preInfusionStartTime = millis();

  // Reset PID integral at brew start to prevent windup carryover between brew cycles
  resetPidIntegral();
  Serial.println("PID integral reset for new brew cycle");

  // Auto-tare scale at brew start (as per requirements: "Brew start should reset shot timer and tare scale")
  if (scaleConnected) {
    tareScale();
    Serial.println("Scale auto-tared for new brew");
  } else {
    Serial.println("Scale not connected - manual tare recommended");
  }

  // Reset scale stability variables for new brew
  // Note: These are static variables in readScale() function that need reset

  // Open brew solenoid to start water flow
  set_brew_solenoid(true);
  // Start pump for pre-infusion (will be modulated)
  set_pump(true);

  Serial.printf("Brew routine started - Target: %.1fg / %.1fs, PID Boost: %.1fs, Pre-infusion: %.1fs\n",
                shotTargetWeight, shotTargetTime, pidBoostDuration, preInfusionDuration);
}

void stopBrewRoutine() {
  if (!brewActive) return;

  brewActive = false;

  // Restore original setpoint temperature
  setSetpointTemp(originalSetpointTemp);
  Serial.printf("Restoring original setpoint: %.1f°F\n", getSetpointTemp());

  // Stop all brew phases
  setPidBoostActive(false);
  setPreInfusionPhase(false);
  setPreInfusionActive(false);

  // Stop pump and close brew solenoid
  set_pump(false);
  set_brew_solenoid(false);

  // Freeze Shot Timer / Weight on OLED/Web UI (per requirements)
  Serial.printf("Brew routine stopped. Final - Time: %.1fs, Weight: %.1fg\n", shotTime, shotWeight);
  Serial.printf("Targets were: Weight: %.1fg, Time: %.1fs\n", shotTargetWeight, shotTargetTime);

  // Reset scale validation for next brew attempt
  scaleValidationWeight = 0.0;
  scaleValidationStartTime = 0;
  scaleValidationCount = 0;
}

void startCleaningCycle() {
  if (currentState != STATE_IDLE) {
    Serial.println("Cannot start cleaning - system not in IDLE state");
    return;
  }

  if (cleaningActive) {
    Serial.println("Cleaning cycle already active");
    return;
  }

  // Initialize cleaning cycle
  cleaningActive = true;
  cleaningCycle = 1;
  cleaningStartTime = millis();
  cleaningPumpState = true;
  cleaningPumpOnTime = millis();
  pumpFailureCount = 0;  // Reset failure count on new start
  pumpTimeoutTriggered = false;

  // Start first cycle - pump and brew solenoid ON together for backflush
  set_brew_solenoid(true);
  set_pump(true);

  // Change to cleaning state
  changeState(STATE_CLEANING);

  Serial.printf("Cleaning cycle started - %d cycles of 10s ON/10s OFF\n", cleaningCycleCount);
}

void stopCleaningCycle() {
  if (!cleaningActive) return;

  // Stop pump and brew solenoid immediately
  set_pump(false);
  set_brew_solenoid(false);
  cleaningPumpState = false;

  // Save cycle count before reset for logging
  int completedCycles = cleaningCycle;

  // Reset cleaning variables
  cleaningActive = false;
  cleaningCycle = 0;
  cleaningStartTime = 0;
  cleaningPumpOnTime = 0;
  cleaningPumpOffTime = 0;
  pumpFailureCount = 0;

  // Clear cleaning system message
  if (systemMessage.startsWith("CLEANING")) {
    systemMessage = "";
  }

  Serial.printf("Cleaning cycle stopped - completed %d cycles\n", completedCycles);
}


void checkFillLevel() {
  fillProbeWet = readFillProbe();

  // Block fill during cleaning cycle
  if (cleaningActive) {
    return;
  }

  // Only start fill routine if in IDLE state, probe is dry, and no calibration active
  if (!fillProbeWet && currentState == STATE_IDLE && !fillInProgress && !isPressureCal()) {
    startFillRoutine();
  } else if (!fillProbeWet && currentState == STATE_IDLE && !fillInProgress && isPressureCal()) {
    // Log fill blocked by pressure calibration (once per check cycle)
    static unsigned long lastFillBlockedLog = 0;
    if (millisElapsed(lastFillBlockedLog, 5000)) {  // Log every 5 seconds max
      Serial.println("Fill routine blocked - pressure calibration in progress");
      lastFillBlockedLog = millis();
    }
  }
}

void startFillRoutine() {
  if (fillInProgress) return;

  fillActive = true;
  fillInProgress = true;
  fillStartTime = millis();
  fillAttemptStartTime = millis();

  // First fill attempt after boot gets the longer startup grace period if
  // the boiler was already filled before this boot (see pendingStartupFillGrace).
  currentFillTimeoutMs = pendingStartupFillGrace ? STARTUP_FILL_TIMEOUT : FILL_MAX_DURATION;
  pendingStartupFillGrace = false;

  // Turn on both solenoid and pump together for first 2s cycle
  set_fill_solenoid(true);
  set_pump(true);

  Serial.printf("Fill routine started - solenoid and pump ON for 2s cycle (timeout: %lus)\n", currentFillTimeoutMs / 1000);
}

void updateFillRoutine() {
  if (!fillInProgress) return;

  // Check for maximum fill timeout (safety protection) - using rollover-safe timing
  // against fillAttemptStartTime (never reset mid-attempt — see its declaration).
  if (millisElapsed(fillAttemptStartTime, currentFillTimeoutMs)) {
    // Timeout reached - stop fill routine and log error
    set_fill_solenoid(false);
    set_pump(false);
    fillActive = false;
    fillInProgress = false;
    Serial.printf("Fill routine timeout after %lu seconds - stopped for safety\n", currentFillTimeoutMs / 1000);
    return;
  }

  // Check if water detected
  if (readFillProbe()) {
    // Water detected - stop fill routine
    set_fill_solenoid(false);
    set_pump(false);
    fillActive = false;
    fillInProgress = false;
    fillProbeWet = true;
    // Mark boiler as filled for first-run setup
    if (!boilerFilled) {
      boilerFilled = true;
      eepromDirty = true;
    }
    Serial.println("Fill routine completed - water detected");
    return;
  }

  // Check if 2s pump cycle completed
  if (millisElapsed(fillStartTime, FILL_PULSE_DURATION)) {
    // Stop both pump and solenoid while reading probe
    set_pump(false);
    set_fill_solenoid(false);
    Serial.println("2s pump cycle completed - both OFF, checking probe");

    // Check probe immediately after stopping pump/solenoid
    if (readFillProbe()) {
      // Water detected - complete fill routine
      fillActive = false;
      fillInProgress = false;
      fillProbeWet = true;
      // Mark boiler as filled for first-run setup
      if (!boilerFilled) {
        boilerFilled = true;
        recomputeSetupComplete();
        eepromDirty = true;
      }
      Serial.println("Fill routine completed - water detected");
    } else {
      // Still dry - restart both solenoid and pump for next 2s cycle
      fillStartTime = millis();
      set_fill_solenoid(true);
      set_pump(true);
      Serial.println("Probe still dry - starting next 2s cycle (both ON)");
    }
  }
}

// checkPIDTiming()/pid_step()/updatePidControl() now live in pid_control.cpp,
// called directly from MainTask's loop — no separate task.

// --- Performance Monitoring Functions (memory/web metrics stay on Core 0) ---
void checkMemoryUsage() {
  if (!performanceMonitoringEnabled) return;

  unsigned long currentTime = millis();
  if (currentTime - lastMemoryCheck < MEMORY_CHECK_INTERVAL) return;

  lastMemoryCheck = currentTime;
  size_t freeHeap = ESP.getFreeHeap();

  // Track minimum free heap
  if (freeHeap < minFreeHeap) {
    minFreeHeap = freeHeap;
  }

  // Memory warning
  if (freeHeap < MEMORY_WARNING_THRESHOLD) {
    if (!memoryWarningActive) {
      Serial.printf("⚠️ LOW MEMORY WARNING: %u bytes free (min: %u)\n",
                    freeHeap, MEMORY_WARNING_THRESHOLD);
      memoryWarningActive = true;
    }
  } else if (memoryWarningActive && freeHeap > (MEMORY_WARNING_THRESHOLD + 5000)) {
    // Clear warning with 5KB hysteresis
    memoryWarningActive = false;
    Serial.printf("✅ Memory usage normalized: %u bytes free\n", freeHeap);
  }
}

void logPerformanceMetrics() {
  if (!performanceMonitoringEnabled) return;

  static unsigned long lastPerfLog = 0;
  if (millis() - lastPerfLog < 30000) return;  // Log every 30 seconds

  lastPerfLog = millis();

  Serial.println("=== PERFORMANCE METRICS ===");
  Serial.printf("PID Timing: Max jitter %lums, Warning count: %lu\n", maxPIDJitter, pidJitterCount);
  Serial.printf("Memory: Current %lu bytes, Minimum %lu bytes\n", (unsigned long)ESP.getFreeHeap(), (unsigned long)minFreeHeap);
  Serial.printf("Web Requests: Total %lu, Avg response: %lums\n",
                webRequestCount,
                webRequestCount > 0 ? totalWebResponseTime / webRequestCount : 0);
  Serial.printf("WebSocket Messages: %lu sent\n", webSocketMessageCount);
  Serial.printf("Active Warnings: PID=%s, Memory=%s\n",
                pidTimingWarningActive ? "YES" : "NO",
                memoryWarningActive ? "YES" : "NO");
  Serial.println("==========================");
}

// writeFloatToEEPROM()/readFloatFromEEPROM()/writeUint16ToEEPROM()/
// readUint16FromEEPROM()/saveParametersToEEPROM()/loadParametersFromEEPROM()/
// initEEPROM() now live in eeprom_store.cpp.

// All web server handlers, webSocketEvent(), buildStatusJSON(),
// sendWebSocketData(), and broadcastWebSocketData() now live in
// web_api.cpp.
// --- Setup & Loop ---
void setup() {
  Serial.begin(115200);

#ifndef BENCH_MODE
  // --- PRODUCTION: Full hardware initialization ---
  // Initialize relay outputs (all OFF for safety)
  Serial.println("Initializing relay outputs...");
  initRelays();
  Serial.println("Relay outputs initialized (all OFF)");

  // Initialize the hardware watchdog subsystem (global init only — the task
  // that actually registers itself with esp_task_wdt_add() is mainTask(),
  // once it starts running at the end of setup(); registering here would
  // register whichever task is executing setup(), not the task that goes on
  // to call esp_task_wdt_reset(), which is exactly the mismatch that used to
  // leave the watchdog unfed).
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = LOOP_TIMEOUT,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
#else
  esp_task_wdt_init(LOOP_TIMEOUT / 1000, true);
#endif

  // Initialize I2C and OLED Display
  if (!Wire.begin()) {
    i2cFailed = true;
    setError(ERR_FLAG_I2C);
    Serial.println("ERROR: I2C initialization failed - temperature sensor may be affected");
  } else {
    Serial.println("I2C initialized successfully");
  }
  initDisplay();

  // Initialize EEPROM and load saved parameters FIRST — fills calibration values
  // (fillProbeThreshold, boilerFilled, etc.) used by hardware init functions below
  initEEPROM();

  // Initialize NAU7802 Scale (24-bit ADC load cell via I2C)
  initScale();

  // Initialize Fill System — uses fillProbeThreshold loaded from EEPROM above
  initFillSystem();

  // Non-blocking startup fill: rather than running a dedicated blocking loop
  // here (which used to hold up WiFi/web server startup for up to 2 minutes
  // on every boot where the steam boiler was dry), let the normal FILL state
  // machine handle it once MainTask starts — same behavior, just not
  // blocking. If the probe is already wet there's nothing to fill, so mark
  // boilerFilled directly (matching the old "already filled" branch).
  if (fillProbeWet) {
    Serial.println("Steam boiler already filled - no startup fill needed");
    if (!boilerFilled) {
      boilerFilled = true;
      eepromDirty = true;
    }
  } else if (boilerFilled) {
    // Boiler was filled before this boot but is dry now — give the first
    // fill attempt the longer STARTUP_FILL_TIMEOUT grace period.
    Serial.println("Steam boiler needs water - will fill non-blocking once running (120s grace)");
    pendingStartupFillGrace = true;
  } else {
    Serial.println("Startup fill: fresh install — fill system will use normal 60s timeout");
    Serial.println("Complete fill probe calibration via web UI for best accuracy");
  }

  // Initialize Brew Switch
  initBrewSwitch();

  // Initialize Pressure Measurement System
  initPressure();

  // Initialize MAX31865 RTD sensor
  initRTDSensor();

  // PID control no longer has its own task to initialize — it's
  // updatePidControl(), called from mainTask()'s loop (see below).
#else
  // --- BENCH MODE: Skip hardware, heater blocked in set_boiler_element() ---
  Serial.println("========================================");
  Serial.println("  BENCH MODE - Heater output BLOCKED");
  Serial.println("  Hardware init skipped, WebUI active");
  Serial.println("========================================");
#endif

  // WiFi AP mode for basic control (change to station if desired)
  WiFi.softAP(ssid, password);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/setParams", handleSetParams);
  server.on("/set", handleSetpoint);
  server.on("/setDose", handleSetDoseWeight);
  server.on("/setPID", handleSetPIDParameters);
  server.on("/tare", handleTare);
  server.on("/calibrate/status", handleCalibrateStatus);
  server.on("/calibrate/fill/start", handleCalibrateFillStart);
  server.on("/calibrate/fill/next", handleCalibrateNext);
  server.on("/calibrate/scale/start", handleCalibrateScaleStart);
  server.on("/calibrate/scale/complete", handleCalibrateScaleComplete);
  server.on("/calibrate/pressure/start", handleCalibratePressureStart);
  server.on("/calibrate/pressure/next", handleCalibratePressureNext);
  server.on("/calibrate/pressure/pump/start", handleCalibratePumpStart);
  server.on("/calibrate/pressure/pump/stop", handleCalibratePumpStop);
  server.on("/calibrate/pressure/set", handleSetPressureCalibrationValue);
  server.on("/calibrate/temp/complete", handleCalibrateTempComplete);
  server.on("/calibrate/cancel", handleCalibrateCancel);
  server.on("/cleaning/start", handleStartCleaning);
  server.on("/cleaning/stop", handleStopCleaning);
  server.on("/setSafety", handleSetSafety);

  // Start web server and WebSocket server
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Web server initialization doesn't return status, but we can detect issues later
  // Server failure will be detected if web requests start failing
  Serial.println("Web server and WebSocket server started successfully");

  lastDisplayUpdate = millis();
  lastScaleRead = millis();
  lastFillCheck = millis();
  systemStartTime = millis();

  // Explicitly pin the entire steady-state workload — PID control included —
  // to one named task on one core, instead of relying on wherever the
  // Arduino framework's own setup()/loop() task happens to run. Pinned to
  // Core 1 specifically to keep it away from the internal WiFi/BT tasks
  // ESP-IDF typically runs on Core 0, not because anything here needs
  // isolation from another *application* task — there isn't a second one
  // anymore. Arduino's loop() becomes an inert stub after this.
  BaseType_t mainTaskResult = xTaskCreatePinnedToCore(
    mainTask,          // Task function
    "MainTask",        // Task name
    8192,              // Stack size (bytes) — web server + display + sensors + PID
    NULL,              // Parameters
    1,                 // Priority — nothing else at the application level to preempt
    &mainTaskHandle,   // Task handle
    1                  // Core 1 — explicit, never assumed
  );
  if (mainTaskResult != pdPASS) {
    Serial.println("ERROR: Failed to create MainTask");
  }

  // --- REQUIREMENTS-COMPLIANT SETUP COMPLETE ---
  Serial.println("System ready:");
  Serial.println("- MainTask: PID control, web UI, display, sensors, state machine");
  Serial.println("  (single task, single core — see architecture analysis for why)");
}

// --- Core Loop Functions ---
void handlePIDUpdates() {
  // Refreshes the cached raw temperature that updatePidControl() consumes.
  // Kept as a separate 50ms-cadence cache (rather than having PID read the
  // RTD directly every 100ms) specifically so PID's EMA filter keeps seeing
  // the same input cadence it always has — changing that would change the
  // filter's effective time constant and PID's control behavior. This was
  // originally about keeping SPI access off a separate PID core; now that
  // there's one task, the cadence-preservation reason is the one that
  // still matters.

  static unsigned long lastTempCache = 0;
  unsigned long now = millis();

  // Update cached temperature at 50ms intervals (faster than PID 100ms cycle)
  if (now - lastTempCache >= 50) {
    lastTempCache = now;
    
    // Only cache temperature if RTD sensor is working (not a hardware fault)
    uint8_t rtdErrorFlag = getErrorFlags() & ERR_FLAG_RTD;
    if (rtdErrorFlag == 0) {
      double newTemp = read_boiler_temp();

      // Reject readings below minTemp (sensor fault / disconnected probe)
      // BUT always cache over-temperature readings so PID task sees actual temp
      // and its safety checks can respond immediately
      if (newTemp >= minTemp) {
        setCachedRawTemp(newTemp);
      } else {
        Serial.printf("Temperature cache update skipped - below minimum: %.1f°F\n", newTemp);
      }
    }
  }
}

void handleNetworkCommunication() {
  // Handle web server requests with rate limiting
  server.handleClient();
  // Update rate limiting timestamp after each request
  lastWebRequest = millis();

  // Handle WebSocket events
  webSocket.loop();

  // Broadcast WebSocket updates at regular intervals
  if (millisElapsed(lastWebSocketBroadcast, WEBSOCKET_BROADCAST_INTERVAL)) {
    lastWebSocketBroadcast = millis();
    broadcastWebSocketData();
  }
}

void handleSystemMonitoring() {
  // Check system errors and loop timing
  checkWatchdog();
  checkSystemHealth();
  handleSystemErrors();

  // Performance monitoring
  checkMemoryUsage();
  logPerformanceMetrics();

  // Process PID task log flags (Serial access from main loop only)
  processPIDTaskLogs();

  // Deferred EEPROM save — coalesces rapid parameter changes, reduces flash wear
  static unsigned long lastEepromSave = 0;
  if (eepromDirty && millisElapsed(lastEepromSave, 2000)) {
    saveParametersToEEPROM();
    eepromDirty = false;
    lastEepromSave = millis();
  }

  // Feed hardware watchdog
  esp_task_wdt_reset();
}

// Process log flags set by PID task (keeps Serial access on main loop)
void processPIDTaskLogs() {
  if (pidLogTempRateError) {
    pidLogTempRateError = false;
    Serial.printf("PID TEMPERATURE RATE ERROR: %.1f°F/s (max: 5.0°F/s)\n", pidLogTempRate);
  }

  if (pidLogTimingWarning) {
    pidLogTimingWarning = false;
    Serial.printf("⚠️ PID TIMING WARNING: %lums cycle (target: 100ms) - Count: %lu\n",
                  pidLogJitter, pidLogJitterCount);
  }

  if (pidLogTimingNormalized) {
    pidLogTimingNormalized = false;
    Serial.println("✅ PID timing normalized");
  }
}

void handleSensorReadings() {
  // Periodic scale reading with adaptive timing
  unsigned long scaleInterval = (currentState == STATE_BREW_START || currentState == STATE_BREW_ACTIVE || currentState == STATE_BREW_END) ? SCALE_READ_INTERVAL_BREW : SCALE_READ_INTERVAL_IDLE;

  if (millisElapsed(lastScaleRead, scaleInterval)) {
    lastScaleRead = millis();
    readScale();
    calculateFlowRate();   // Calculate flow rate after scale reading
    calculateShotRatio();  // Calculate shot ratio after scale reading
  }

  // Pressure reading and calibration handling
  if (millisElapsed(lastPressureRead, PRESSURE_READ_INTERVAL)) {
    lastPressureRead = millis();
    readPressure();
  }
}

void handleUserInterface() {
  // Adaptive display updates based on system state
  unsigned long currentDisplayInterval;
  if (currentState == STATE_BREW_ACTIVE || currentState == STATE_BREW_START || currentState == STATE_BREW_END) {
    currentDisplayInterval = DISPLAY_UPDATE_BREW;  // Fast updates during brew
  } else if (hasError()) {
    currentDisplayInterval = DISPLAY_UPDATE_ERROR;  // Medium updates during error
  } else {
    currentDisplayInterval = DISPLAY_UPDATE_IDLE;  // Slow updates when idle
  }

  if (millisElapsed(lastDisplayUpdate, currentDisplayInterval)) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}

void handleSystemOperations() {
  // Clear completion message after 5 seconds
  if (completionMessageActive && millisElapsed(completionMessageTime, 5000)) {
    systemMessage = "";
    completionMessageTime = 0;
    completionMessageActive = false;
  }

  // Fill level checking and routine
  if (millisElapsed(lastFillCheck, FILL_CHECK_INTERVAL)) {
    lastFillCheck = millis();
    checkFillLevel();
    updateFillRoutine();
  }

  // Brew switch monitoring
  checkBrewSwitch();

  // Calibration steps are processed only when triggered by web handlers
  // (handleCalibrateNext, handleCalibratePressureNext) to prevent auto-advancing
}

void handleBrewPhaseManagement() {
  // Brew timing and phase management
  if (currentState == STATE_BREW_ACTIVE) {
    // Update shot timing (rollover-safe)
    // Unsigned arithmetic automatically handles rollover correctly
    unsigned long elapsedMillis = millis() - brewStartTime;
    shotTime = (double)elapsedMillis / 1000.0;

    // Handle PID boost timing
    if (getPidBoostActive() && millisElapsed(pidBoostStartTime, pidBoostDuration * 1000)) {
      setPidBoostActive(false);
    }

    // Handle pre-infusion timing and pump modulation
    if (getPreInfusionPhase()) {
      if (millisElapsed(preInfusionStartTime, preInfusionDuration * 1000)) {
        // Pre-infusion phase completion
        setPreInfusionPhase(false);
        set_pump(true);  // Full pump after pre-infusion
      } else {
        // Configurable pulse during pre-infusion
        unsigned long cycleDuration = (unsigned long)(preInfusionPulseRate * 2);  // Full on/off cycle
        unsigned long pulseTime = (millis() - preInfusionStartTime) % cycleDuration;
        set_pump(pulseTime < preInfusionPulseRate);
      }
    }
  }
}

void handleSerialCommands() {
  // Handle serial commands with rate limiting
  static unsigned long lastSerialCommand = 0;
  const unsigned long SERIAL_RATE_LIMIT = 100;  // 100ms between commands

  if (Serial.available() && millisElapsed(lastSerialCommand, SERIAL_RATE_LIMIT)) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    lastSerialCommand = millis();

    if (line.startsWith("SETPOINT=")) {
      float newSetpoint = line.substring(9).toFloat();
      if (newSetpoint >= 180.0 && newSetpoint <= 220.0) {
        setSetpointTemp(newSetpoint);
        originalSetpointTemp = newSetpoint;  // Update original for future compensation
        eepromDirty = true;
        Serial.printf("Setpoint set to %.1fF\n", newSetpoint);
      }
    } else if (line.startsWith("TARGET_WEIGHT=")) {
      float newWeight = line.substring(14).toFloat();
      if (newWeight >= 10.0 && newWeight <= 100.0) {
        shotTargetWeight = newWeight;
        eepromDirty = true;
        Serial.printf("Target weight set to %.1fg\n", newWeight);
      }
    } else if (line.startsWith("TARGET_TIME=")) {
      float newTime = line.substring(12).toFloat();
      if (newTime >= 10.0 && newTime <= 60.0) {
        shotTargetTime = newTime;
        // Validate and adjust dependent parameters
        if (preInfusionDuration > shotTargetTime) {
          preInfusionDuration = shotTargetTime;
          Serial.printf("Pre-infusion duration adjusted to %.1fs (shot target time)\n", preInfusionDuration);
        }
        if (pidBoostDuration > shotTargetTime) {
          pidBoostDuration = shotTargetTime;
          Serial.printf("PID Boost duration adjusted to %.1fs (shot target time)\n", pidBoostDuration);
        }
        eepromDirty = true;
        Serial.printf("Target time set to %.1fs\n", newTime);
      }
    } else if (line.startsWith("PID_BOOST=")) {
      float newBoost = line.substring(10).toFloat();
      if (newBoost >= 0.0 && newBoost <= shotTargetTime) {
        pidBoostDuration = newBoost;
        eepromDirty = true;
        Serial.printf("PID Boost duration set to %.1fs\n", newBoost);
      } else {
        Serial.printf("PID Boost duration must be 0-%.1fs (shot target time)\n", shotTargetTime);
      }
    } else if (line.startsWith("PREINFUSION=")) {
      float newPreInf = line.substring(12).toFloat();
      if (newPreInf >= 0.0 && newPreInf <= shotTargetTime) {
        preInfusionDuration = newPreInf;
        eepromDirty = true;
        Serial.printf("Pre-infusion duration set to %.1fs\n", newPreInf);
      } else {
        Serial.printf("Pre-infusion duration must be 0-%.1fs (shot target time)\n", shotTargetTime);
      }
    } else if (line.startsWith("PID_BYPASS=")) {
      int bypassValue = line.substring(11).toInt();
      bool enableBypass = (bypassValue == 1);
      setAllowUnsafePid(enableBypass);
      eepromDirty = true;
      if (enableBypass) {
        Serial.println("WARNING: PID bypass enabled - PID may run before setup completes");
      } else {
        Serial.println("PID bypass disabled - setup required for PID");
      }
    } else if (line.startsWith("CLEANING_CYCLES=")) {
      int newCycles = line.substring(16).toInt();
      if (newCycles >= 1 && newCycles <= 20) {
        cleaningCycleCount = newCycles;
        eepromDirty = true;
        Serial.printf("Cleaning cycle count set to %d\n", newCycles);
      } else {
        Serial.println("Cleaning cycles must be 1-20");
      }
    } else if (line == "TARE") {
      tareScale();
    } else if (line == "SAVE") {
      saveParametersToEEPROM();
    } else if (line == "LOAD") {
      loadParametersFromEEPROM();
    } else if (line == "STATUS") {
      double temp = getTemp();
      double setpoint = getSetpointTemp();
      Serial.printf("Temp: %.1fF SP: %.1fF Boiler: %d Pump: %d BrewSol: %d FillSol: %d Weight: %.1f Scale: %s\n",
                    temp, setpoint,
                    digitalRead(PIN_RELAY_CH4), digitalRead(PIN_RELAY_CH2),
                    digitalRead(PIN_RELAY_CH1), digitalRead(PIN_RELAY_CH3),
                    shotWeight, scaleConnected ? "OK" : "ERR");
    } else if (line == "PERF_ON") {
      performanceMonitoringEnabled = true;
      Serial.println("Performance monitoring enabled");
    } else if (line == "PERF_OFF") {
      performanceMonitoringEnabled = false;
      Serial.println("Performance monitoring disabled");
    } else if (line == "PERF_RESET") {
      // Reset performance counters
      maxPIDJitter = 0;
      pidJitterCount = 0;
      minFreeHeap = SIZE_MAX;
      webRequestCount = 0;
      webSocketMessageCount = 0;
      maxWebResponseTime = 0;
      totalWebResponseTime = 0;
      pidTimingWarningActive = false;
      memoryWarningActive = false;
      Serial.println("Performance counters reset");
    } else if (line == "PERF_REPORT") {
      logPerformanceMetrics();
    }
  }
}

// --- MainTask: the only task running the steady-state workload ---
// Explicitly pinned (see setup()) instead of relying on the Arduino
// framework's default placement for its own loop() task. Everything here
// was previously split across two tasks (a "SystemTask" doing this same
// sequence, and a separate "ControlTask" running updatePidControl()'s logic
// independently at 100ms on its own core) — folded into one task, one core,
// since PID's actual per-cycle work is microseconds and never justified a
// dedicated core; see the architecture analysis this session for the full
// reasoning. updatePidControl() is internally gated to 100ms (same
// self-gating pattern as handlePIDUpdates()/handleSensorReadings() below),
// so calling it unconditionally here every iteration is correct.
// (mainTaskHandle is declared near the top of the file, alongside the other
// globals, since setup() references it before this point.)
void mainTask(void* parameter) {
  Serial.printf("MainTask started on core %d\n", xPortGetCoreID());

#ifndef BENCH_MODE
  // Register THIS task with the watchdog — must happen here, not in
  // setup(), because esp_task_wdt_reset() below only feeds the watchdog
  // entry for whichever task calls it. Registering from setup() would
  // register the Arduino main/loop task instead, which never calls reset()
  // again once it falls into the inert loop() below — leaving the real
  // watchdog entry unfed and causing a panic/reboot every LOOP_TIMEOUT.
  // Guarded by BENCH_MODE to match esp_task_wdt_init() in setup(), which is
  // also skipped in bench mode — calling add() before init() would just log
  // a harmless warning, but BENCH_MODE is meant to have no watchdog at all.
  if (esp_task_wdt_add(NULL) != ESP_OK) {
    Serial.println("WARNING: Failed to add MainTask to watchdog");
  }
#endif

  for (;;) {
    handleNetworkCommunication();  // Network: Web server and WebSocket handling
#ifndef BENCH_MODE
    handlePIDUpdates();            // Critical: temperature reading + caching for PID
    updatePidControl();            // Critical: PID control (self-gated to 100ms)
    handleSystemMonitoring();      // Safety: Error monitoring and watchdog feed
    updateStateMachine();          // Control: State machine updates
    handleSensorReadings();        // Sensors: Scale and pressure readings
    handleUserInterface();         // Display: OLED updates
    handleSystemOperations();      // Operations: Fill, brew switch, calibration
    handleBrewPhaseManagement();   // Brew: Shot timing and phase control
#endif
    handleSerialCommands();        // Debug: Serial command processing

    // Yield to the scheduler/idle task each cycle — same effect as the small
    // implicit yield the Arduino loopTask gets between loop() calls.
    vTaskDelay(1);
  }
}

// Arduino's own setup()/loop() task is no longer where the steady-state
// workload runs (see mainTask above) — loop() is intentionally inert.
void loop() {
  vTaskDelay(portMAX_DELAY);
}

// initPressure()/readPressure() now live in sensors.cpp.

// startPressureCalibration()/processPressureCalibrationStep()/
// completePressureCalibration() now live in calibration.cpp.

/*
================================================================================
DEVELOPMENT REFERENCE DOCUMENTATION
================================================================================
// ESP.getFreeHeap() < 10000  // Less than 10KB = WARNING, 50-80KB free heap = OK

SERIAL COMMANDS:
================================================================================
Commands are case-sensitive. Send via Serial Monitor at 115200 baud.

TEMPERATURE CONTROL:
  SETPOINT=###.#        - Set brew temperature (180.0-220.0°F)
                         Example: SETPOINT=200.5

BREW PARAMETERS:
  TARGET_WEIGHT=##.#    - Set target shot weight (10.0-100.0g)
                         Example: TARGET_WEIGHT=36.0
  TARGET_TIME=##.#      - Set target shot time (10.0-60.0s)
                         Example: TARGET_TIME=30.0
  PID_BOOST=##.#        - Set PID boost duration (0.0-shot_time s)
                         Example: PID_BOOST=4.0
  PREINFUSION=##.#      - Set pre-infusion duration (0.0-shot_time s)
                         Example: PREINFUSION=8.0

SAFETY:
  PID_BYPASS=0|1         - Allow PID before setup completes (use with caution)

SCALE OPERATIONS:
  TARE                  - Tare the scale to zero weight
  
SYSTEM COMMANDS:
  SAVE                  - Save current parameters to EEPROM
  LOAD                  - Load parameters from EEPROM
  STATUS                - Display comprehensive system status
  
PERFORMANCE MONITORING:
  PERF_ON               - Enable performance monitoring and alerts
  PERF_OFF              - Disable performance monitoring
  PERF_RESET            - Reset all performance counters to zero
  PERF_REPORT           - Force immediate performance metrics report

RESPONSES:
  - Invalid commands return error messages
  - Valid commands return confirmation
  - Parameter constraints are enforced
  - Changes are automatically saved to EEPROM

USAGE EXAMPLES:
  SETPOINT=201.5        Set brew temperature to 201.5°F
  TARGET_WEIGHT=34.0    Set target shot weight to 34g
  PID_BOOST=5.0         Set PID boost to 5 seconds
  TARE                  Zero the scale
  PID_BYPASS=1          Enable PID bypass during setup (unsafe)
  PERF_ON               Start performance monitoring
  STATUS                Show current system state

EEPROM MEMORY MAP:
================================================================================
Total Size: 72 bytes (addresses 0-71)
Signature: 0xABCD (validates EEPROM integrity)

Address Range | Size | Parameter              | Type    | Default
--------------|------|------------------------|---------|----------
0-1           | 2    | EEPROM Signature       | uint16  | 0xABCD
2-5           | 4    | Brew Temperature       | float   | 200.0°F
6-9           | 4    | Shot Target Time       | float   | 30.0s
10-13         | 4    | Shot Target Weight     | float   | 36.0g
14-17         | 4    | Pre-infusion Duration  | float   | 8.0s
18-21         | 4    | Scale Calibration      | float   | 1.0
22-25         | 4    | PID Boost Duration     | float   | 4.0s
26-29         | 4    | PID Kp                 | float   | 40.0
30-33         | 4    | PID Ki                 | float   | 0.8
34-37         | 4    | PID Kd                 | float   | 120.0
38-41         | 4    | Fill Probe Threshold   | float   | 512
42-45         | 4    | Pre-infusion Pulse Rate| float   | 500ms
46-49         | 4    | Pressure Offset        | float   | 0.0
50-53         | 4    | Pressure Scale         | float   | 1.0
54-57         | 4    | Dose Weight            | float   | 18.0g
58-61         | 4    | Scale Zero Offset      | int32_t | 0
62            | 1    | Setup Complete         | byte    | 0
63            | 1    | Temp Cal Complete      | byte    | 0
64            | 1    | Pressure Cal Complete  | byte    | 0
65            | 1    | Boiler Filled          | byte    | 0
66            | 1    | Allow PID Bypass       | byte    | 0
67            | 1    | Scale Cal Complete     | byte    | 0
68-71         | 4    | Cleaning Cycle Count   | int     | 10

EEPROM OPERATIONS:
- Automatic save on parameter changes via serial/web
- Automatic load on system startup
- Signature verification prevents corruption
- Write protection during active operations (brew/fill)
- Wear optimization: save only on completion, not intermediate steps

EEPROM ADDRESSES USED: 0-71 (72 bytes total)
EEPROM ADDRESSES FREE: 72-4095 (4024 bytes available for future features)

================================================================================

Before Hardware Upgrade:
------------------------
Increase PID timer priority in timer config
Reduce WebSocket broadcast frequency (2Hz → 1Hz)
Implement client-side caching for status updates
Optimize chart data structure (circular buffer)
Add request rate limiting on web endpoints

✅ Good Performance:
PID jitter: <5ms consistently
Memory usage: >50KB free consistently
Web response: <100ms average
No warnings in console


*/
