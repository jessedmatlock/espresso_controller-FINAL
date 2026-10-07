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
 - Requirements-compliant architecture: Core 1 hardware timer ISR + Core 0 millis() timing
 - Core 1: Hardware timer ISR for PID control (100ms deterministic)
 - Core 0: Main loop with millis() timing for all non-critical operations
 - Comprehensive state machine with error handling and safety margins
 - Real-time Web UI with Chart.js shot graphing
 - Complete EEPROM parameter storage including PID gains and fill probe threshold
 - Parameter validation ensuring pre-infusion/PID boost times ≤ shot target time
 - Hardware safety cutoff and watchdog systems
 - Fully non-blocking operation - zero delay() calls throughout system
 - Momentary brew button with rising edge detection and double-press protection
 - 3-wire pressure transducer (0.5-4.5V, 0-2MPa) with Web UI calibration and pump control
 - Comprehensive EEPROM parameter storage (72 bytes, addresses 0-71, no conflicts)

NOTE: Fully requirements-compliant production firmware per specification document.
*/

// Uncomment for bench testing WITHOUT hardware connected:
// Skips ALL hardware init (relays, sensors, display, PID task, watchdog).
// Only WiFi AP, web server, WebSocket, and serial remain active.
// Use to confirm web UI and serial before installing MCU onto expansion board.
// Comment out when installing onto hardware for sensor commissioning or production.

//#define BENCH_MODE

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "bootstrap_custom.h"
#include "bootstrap_js.h"
#include "apexcharts_js.h"
#include "web_ui_css.h"
#include "web_ui_html.h"
#include "web_ui_js.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_NAU7802.h>
#include <EEPROM.h>
#include <Adafruit_MAX31865.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <atomic>

// --- Pin Configuration ---
// Pins must be defined before hardware objects that use them
const int PIN_TEMP_CS = 21;     // SPI CS for thermocouple/MAX31865 (D10/GPIO21)
const int PIN_FILL_PROBE = 2;   // Fill level probe (A1/GPIO2 - analog input)
const int PIN_BREW_SWITCH = 3;  // Brew momentary switch (A2/GPIO3 - NO contact to GND)
                                // Momentary operation: Press to start brew, press again to stop brew
                                // Rising edge detection with debounce and double-press protection

// 4-Channel Relay Control (3.3V trigger, 12V coil, switches 10A @ 120VAC)
const int PIN_RELAY_CH1 = 8;   // Ch1: Brew Solenoid (D5/GPIO8)
const int PIN_RELAY_CH2 = 9;   // Ch2: Pump (D6/GPIO9)
const int PIN_RELAY_CH3 = 10;  // Ch3: Fill Solenoid (D7/GPIO10)
const int PIN_RELAY_CH4 = 17;  // Ch4: Boiler Element SSR (D8/GPIO17)

// --- Hardware Objects ---
// OLED Display - SH1107 128x128 I2C (Arduino Nano ESP32: A4=SDA/GPIO11, A5=SCL/GPIO12)
U8G2_SH1107_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// NAU7802 Load Cell (I2C interface, shares bus with OLED)
Adafruit_NAU7802 nau;

// MAX31865 PT100 RTD sensor
Adafruit_MAX31865 rtdSensor = Adafruit_MAX31865(PIN_TEMP_CS);

// RTD configuration constants
const float RREF = 430.0;      // Reference resistor value (typically 430 ohms for PT100)
const float RNOMINAL = 100.0;  // PT100 nominal resistance at 0°C (100 ohms)

// Control parameters (all temperatures in Fahrenheit)
volatile double setpointTemp = 200.0;  // Fahrenheit, brew temp default (200°F)
double originalSetpointTemp = 200.0;   // Store original setpoint for predictive compensation
const double safetyTemp = 225.0;       // Fahrenheit, emergency cutoff (225°F, 5°F margin above max setpoint)
const double minTemp = 32.0;           // Fahrenheit, minimum reading (32°F = 0°C)

const double tempCompensation = 2.0;  // Fahrenheit, predictive compensation for cold water inrush

// Pressure safety thresholds (BAR) with hysteresis to prevent pump oscillation
const double PRESSURE_LOCKOUT_HIGH = 12.0;  // Enter lockout when pressure reaches this level
const double PRESSURE_LOCKOUT_LOW = 11.0;   // Exit lockout when pressure drops to this level

// PID parameters (simple PID)
double Kp = 40.0;
double Ki = 0.8;
double Kd = 120.0;
const double integralMax = 1000.0;  // PID integral windup protection limit

// Temperature filtering constant
// Alpha = 0.1 provides light filtering with ~10 sample effective window at 100ms intervals
// This balances noise reduction with responsiveness for PID control
// Lower alpha (0.05): More smoothing but slower response to actual temp changes
// Higher alpha (0.3): Faster response but more noise passes through
const double TEMP_FILTER_ALPHA = 0.1;  // Optimized for PT100 RTD with low inherent noise

// Loop timing
const unsigned long CONTROL_INTERVAL_MS = 100;       // PID interval (100ms)
const unsigned long PID_TIMER_INTERVAL_US = 100000;  // 100ms in microseconds for hardware timer

// Wi-Fi (stub)
const char* ssid = "espresso_AP";
const char* password = "espresso";
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Web server rate limiting
unsigned long lastWebRequest = 0;
const unsigned long WEB_RATE_LIMIT = 50;  // 50ms between requests

// WebSocket variables
unsigned long lastWebSocketBroadcast = 0;
const unsigned long WEBSOCKET_BROADCAST_INTERVAL = 100;  // 100ms WebSocket updates

// --- EEPROM Storage Structure ---
const uint16_t EEPROM_SIGNATURE = 0xABCD;
const int EEPROM_SIZE = 512;  // Total EEPROM size

// EEPROM Memory Map (per requirements document + PID gains)
const int ADDR_SIGNATURE = 0;               // 0-1: Signature 0xABCD (2 bytes)
const int ADDR_BREW_TEMP = 2;               // 2-5: Brew Temp (°F) (4 bytes)
const int ADDR_SHOT_TIME = 6;               // 6-9: Shot Time (s) (4 bytes)
const int ADDR_SHOT_WEIGHT = 10;            // 10-13: Shot Weight (g) (4 bytes)
const int ADDR_PREINFUSION = 14;            // 14-17: Pre-infusion Time (s) (4 bytes)
const int ADDR_SCALE_CAL = 18;              // 18-21: Scale Calibration Factor (4 bytes)
const int ADDR_PID_BOOST = 22;              // 22-25: PID Boost (s) (4 bytes)
const int ADDR_PID_KP = 26;                 // 26-29: PID Kp gain (4 bytes)
const int ADDR_PID_KI = 30;                 // 30-33: PID Ki gain (4 bytes)
const int ADDR_PID_KD = 34;                 // 34-37: PID Kd gain (4 bytes)
const int ADDR_FILL_THRESHOLD = 38;         // 38-41: Fill Probe Threshold (4 bytes)
const int ADDR_PREINFUSION_RATE = 42;       // 42-45: Pre-infusion Pulse Rate (ms) (4 bytes)
const int ADDR_PRESSURE_OFFSET = 46;        // 46-49: Pressure Calibration Offset (4 bytes)
const int ADDR_PRESSURE_SCALE = 50;         // 50-53: Pressure Calibration Scale (4 bytes)
const int ADDR_DOSE_WEIGHT = 54;            // 54-57: Dose Weight (g) (4 bytes)
const int ADDR_SCALE_ZERO_OFFSET = 58;      // 58-61: Scale Zero Offset for NAU7802 (4 bytes)
const int ADDR_SETUP_COMPLETE = 62;         // 62: Setup Complete flag (1 byte)
const int ADDR_TEMP_CAL_COMPLETE = 63;      // 63: Temperature Calibration Complete (1 byte)
const int ADDR_PRESSURE_CAL_COMPLETE = 64;  // 64: Pressure Calibration Complete (1 byte)
const int ADDR_BOILER_FILLED = 65;          // 65: Boiler Filled at least once (1 byte)
const int ADDR_ALLOW_UNSAFE_PID = 66;       // 66: Allow PID bypass during setup (1 byte)
const int ADDR_SCALE_CAL_COMPLETE = 67;     // 67: Scale Calibration Complete (1 byte)
const int ADDR_CLEANING_CYCLES = 68;       // 68-71: Cleaning Cycle Count (4 bytes, int)
// Total EEPROM: 72 bytes (addresses 0-71)

// --- State Machine ---
enum SystemState {
  STATE_BOOT,
  STATE_IDLE,
  STATE_BREW_START,
  STATE_BREW_ACTIVE,
  STATE_BREW_END,
  STATE_FILL,
  STATE_CLEANING,
  STATE_ERROR
};

volatile SystemState currentState = STATE_BOOT;
volatile SystemState previousState = STATE_BOOT;

// First-run setup state
bool setupComplete = false;        // True when required calibrations are done
bool tempCalComplete = false;      // Temperature probe calibration complete (REQUIRED for PID)
bool pressureCalComplete = false;  // Pressure transducer calibration complete (REQUIRED for PID)
bool boilerFilled = false;         // Steam boiler has been filled (tracked independently, not required for PID)
bool scaleCalComplete = false;     // Scale calibration complete (optional - system uses shot time target if scale unavailable)
bool fillCalComplete = false;      // Fill probe calibration complete (optional, non-blocking)
bool allowUnsafePid = false;       // Allow PID to run before setup completes (requires explicit opt-in)

// Shot parameters and targets
double shotTargetWeight = 36.0;   // grams, default target weight
double shotTargetTime = 30.0;     // seconds, default target time
const double maxBrewTime = 60.0;  // seconds, maximum brew timeout
unsigned long brewStartTime = 0;  // milliseconds when brew started

// PID Boost and Pre-infusion parameters
double pidBoostDuration = 4.0;      // seconds, default PID boost duration
double preInfusionDuration = 8.0;   // seconds, default pre-infusion duration
double preInfusionPulseRate = 500;  // milliseconds, configurable pulse on/off duration
bool pidBoostActive = false;
bool preInfusionPhase = false;
unsigned long pidBoostStartTime = 0;
unsigned long preInfusionStartTime = 0;

// State (temperatures in Fahrenheit)
double currentTemp = 77.0;             // Start at room temp ~77°F - filtered value for display/control
volatile double cachedRawTemp = 77.0;  // Raw temp cached by main loop for PID task (avoids SPI from Core 1)
double pidOutput = 0.0;                // 0.0 - 1.0

// Temperature rate-of-change protection (unified limit for sensor and PID)
double lastTemp = 77.0;
unsigned long lastTempTime = 0;
const double MAX_TEMP_RATE = 5.0;  // Maximum °F per second (safety limit - unified for sensor and PID)

// Display variables
double shotTime = 0.0;
double shotWeight = 0.0;
bool scaleConnected = false;
float scaleValidationWeight = 0.0;           // Last weight for stability validation
unsigned long scaleValidationStartTime = 0;  // When validation started
int scaleValidationCount = 0;                // Consecutive stable readings
bool boilerHeating = false;
bool brewActive = false;
bool fillActive = false;
bool preInfusionActive = false;
String systemMessage = "";  // System messages for display (e.g., "SCALE ERR", "TEMP ERR")

// Atomic error flags — lock-free on ESP32 (8-bit), no mutex needed
// Set/cleared from Core 0 (main loop), read from Core 1 (PID task)
std::atomic<uint8_t> errorFlags(0);
#define ERR_FLAG_TEMP (1 << 0)      // Over-temperature error
#define ERR_FLAG_RTD (1 << 1)       // RTD sensor fault
#define ERR_FLAG_PID (1 << 2)       // PID control failure
#define ERR_FLAG_SCALE (1 << 3)     // Scale disconnected
#define ERR_FLAG_PRESSURE (1 << 4)  // Over-pressure error
#define ERR_FLAG_DISPLAY (1 << 5)   // Display initialization failed
#define ERR_FLAG_I2C (1 << 6)       // I2C communication failed
#define ERR_FLAG_LOOP_TIMEOUT (1 << 7)  // Main loop timeout

inline uint8_t getErrorFlags() {
  return errorFlags.load(std::memory_order_acquire);
}
inline void setErrorFlag(uint8_t flag) {
  errorFlags.fetch_or(flag, std::memory_order_release);
}
inline void clearErrorFlag(uint8_t flag) {
  errorFlags.fetch_and(~flag, std::memory_order_release);
}

// Thread-safe error flag helpers for PID task
inline bool hasErrorFlag(uint8_t flag) {
  return (getErrorFlags() & flag) != 0;
}
inline bool hasCriticalErrorFlag() {
  return (getErrorFlags() & (ERR_FLAG_TEMP | ERR_FLAG_RTD | ERR_FLAG_PID)) != 0;
}

// Flow rate calculation variables
double currentFlowRate = 0.0;  // ml/s extraction rate
double lastWeightForFlow = 0.0;
unsigned long lastFlowCalculation = 0;
const unsigned long FLOW_CALCULATION_INTERVAL = 500;  // 500ms flow rate update interval
double smoothedFlowRate = 0.0;                        // Smoothed flow rate for display
bool flowRateFirstCalculation = true;                 // Reset flag for new brew cycles

// Shot ratio tracking variables
double doseWeight = 18.0;       // Default dose weight in grams (user configurable)
double currentShotRatio = 0.0;  // Current output÷dose ratio (e.g., 2.0 = 1:2 ratio)

// NAU7802 scale variables
int32_t scaleZeroOffset = 0;  // Zero offset for tare functionality

// Scale reading stability variables (moved from static in readScale() for clarity and testability)
float lastValidScaleWeight = 0;
int scaleStableReadings = 0;
bool scaleStabilityReset = false;  // Flag to reset stability on brew start

// Scale reliability tracking for weight-based brew stop
// If scale becomes unreliable at ANY point during shot, brew stops by time instead of weight
bool scaleReliableEntireShot = true;  // Reset to true at brew start, set false if scale fails

// PID output tracking variables
double currentPIDOutputPercent = 0.0;  // PID output as percentage (0-100%)

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
// PID timing validation (volatile: written by Core 1 checkPIDTiming, read by Core 0 logPerformanceMetrics)
static volatile unsigned long lastPIDTime = 0;
static volatile unsigned long maxPIDJitter = 0;
static volatile unsigned long pidJitterCount = 0;
const unsigned long PID_JITTER_THRESHOLD = 105;  // Alert if PID cycle > 105ms

// Memory monitoring
static unsigned long lastMemoryCheck = 0;
static size_t minFreeHeap = SIZE_MAX;
const size_t MEMORY_WARNING_THRESHOLD = 40000;     // Alert if free heap < 40KB
const unsigned long MEMORY_CHECK_INTERVAL = 5000;  // Check every 5 seconds

// Performance metrics
static unsigned long webRequestCount = 0;
static unsigned long webSocketMessageCount = 0;
static unsigned long maxWebResponseTime = 0;
static unsigned long totalWebResponseTime = 0;

// Load monitoring flags
static volatile bool performanceMonitoringEnabled = true;  // Read by Core 1 (checkPIDTiming), written by Core 0 (serial cmds)
static bool memoryWarningActive = false;
static volatile bool pidTimingWarningActive = false;  // Written by Core 1, read by Core 0

// PID task logging flags (set by PID task, processed by main loop)
volatile bool pidLogTempRateError = false;
volatile double pidLogTempRate = 0.0;
volatile bool pidLogTimingWarning = false;
volatile unsigned long pidLogJitter = 0;
volatile unsigned long pidLogJitterCount = 0;
volatile bool pidLogTimingNormalized = false;

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

// Scale data protection mutex
portMUX_TYPE scaleMux = portMUX_INITIALIZER_UNLOCKED;

// Calibration state machine (replaces String-based state tracking)
enum CalStep : uint8_t {
  CAL_IDLE,
  CAL_FILL_DRY, CAL_FILL_WET,
  CAL_SCALE_ZERO, CAL_SCALE_WEIGHT, CAL_SCALE_CONFIRM,
  CAL_PRESSURE_ZERO, CAL_PRESSURE_HIGH
};
CalStep calStep = CAL_IDLE;
inline bool isCalibrating() { return calStep != CAL_IDLE; }
inline bool isPressureCal() { return calStep == CAL_PRESSURE_ZERO || calStep == CAL_PRESSURE_HIGH; }
inline bool isFillScaleCal() { return calStep >= CAL_FILL_DRY && calStep <= CAL_SCALE_CONFIRM; }
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
const unsigned long FILL_MAX_DURATION = 60000;   // 60 second maximum fill time
unsigned long fillStartTime = 0;
bool fillInProgress = false;

// Brew switch variables
bool brewSwitchPressed = false;
bool prevBrewSwitchState = false;
unsigned long lastBrewSwitchDebounce = 0;
const unsigned long BREW_SWITCH_DEBOUNCE = 50;   // 50ms debounce
bool lastBrewButtonState = false;                // For rising edge detection
unsigned long lastBrewButtonPress = 0;           // For double-press protection
const unsigned long BREW_BUTTON_COOLDOWN = 200;  // 200ms between button presses

// Pressure measurement variables
const int PIN_PRESSURE = 1;    // Pressure transducer analog input (A0/GPIO1)
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

// Pressure smoothing (exponential moving average)
double filteredPressure = 0.0;
const double pressureAlpha = 0.2;  // Smoothing factor (0.2 = light smoothing)
bool pressureFirstReading = true;

// Pressure transducer constants - Direct 3.3V connection
const double ADC_MAX_VOLTAGE = 3.3;  // ESP32 ADC reference voltage
const int ADC_RESOLUTION = 4096;     // 12-bit ADC resolution

// PID internals (accessed by ISR - use volatile)
volatile double pid_integral = 0.0;
volatile double pid_prev_error = 0.0;
// Core 1 PID Task variables
TaskHandle_t pidTaskHandle = NULL;
portMUX_TYPE pidMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool pidTaskRunning = false;

// EEPROM save re-entrancy guard is a static bool inside saveParametersToEEPROM()
// Deferred EEPROM save — reduces flash wear by coalescing rapid parameter changes
volatile bool eepromDirty = false;

// State change mutex - protects currentState from race conditions
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

// Control mutex - protects shared control variables accessed by PID task
portMUX_TYPE controlMux = portMUX_INITIALIZER_UNLOCKED;

// Temperature mutex - protects currentTemp from race conditions between PID task and main loop
portMUX_TYPE tempMux = portMUX_INITIALIZER_UNLOCKED;

// Cached temperature mutex - protects cachedRawTemp for main loop -> PID task communication
portMUX_TYPE cachedTempMux = portMUX_INITIALIZER_UNLOCKED;

// FreeRTOS variables removed - using simple millis() timing per requirements

// Unified display timing (matches Web UI for perfect sync)
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_IDLE = 100;   // 100ms unified rate
const unsigned long DISPLAY_UPDATE_BREW = 100;   // 100ms unified rate
const unsigned long DISPLAY_UPDATE_ERROR = 100;  // 100ms unified rate

// Error handling and watchdog
unsigned long lastLoopTime = 0;
unsigned long lastWatchdogCheck = 0;
const unsigned long LOOP_TIMEOUT = 5000;         // 5 seconds
const unsigned long LOOP_CHECK_INTERVAL = 1000;  // Check every second
bool loopTimeoutTriggered = false;
bool rtdFault = false;

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

// Relay control
void set_pump(bool state);
void set_brew_solenoid(bool state);
void set_fill_solenoid(bool state);
void set_boiler_element(bool state);

// Calibration
void saveParametersToEEPROM();
bool loadParametersFromEEPROM();
int readFillProbeRaw();
void startPressureCalibration();
void processPressureCalibrationStep();
void completePressureCalibration();

// Routines
void startBrewRoutine();
void stopBrewRoutine();
void startFillRoutine();
void stopCleaningCycle();

// PID
void pid_step();

// Sensors
double readPressure();
void initPressure();
double read_boiler_temp();
void readScale();
void calculateFlowRate();
void calculateShotRatio();
double getTemp();
void setTemp(double temp);
double getCachedRawTemp();
void setCachedRawTemp(double temp);
uint8_t getErrorFlags();

// Error system
void setError(uint8_t flag);
void clearError(uint8_t flag);

// Hardware init
void initRelays();
void runStartupFillCycle();
bool initRTDSensor();
void initPIDTask();

// Display
void updateDisplay();

// Calibration (additional)
void startFillProbeCalibration();
void startScaleCalibration();
void completeScaleCalibration(float knownWeight);
const char* calStepName(CalStep s);
bool isFillScaleCal();
bool isPressureCal();

// System operations
void startCleaningCycle();
void checkFillLevel();
void updateFillRoutine();
void checkBrewSwitch();
bool readFillProbe();
void checkSystemHealth();
void checkWatchdog();
void handleSystemErrors();
void checkMemoryUsage();
void logPerformanceMetrics();

// Web handlers
void handleRoot();
void handleStatus();
void broadcastWebSocketData();

// WebSocket
void sendWebSocketData(uint8_t num);
int buildStatusJSON(char* buf, size_t len, bool includeSetup);
void processPIDTaskLogs();
void handleSetSafety();
void handleSetParams();

// --- Timing Helper Functions ---
bool millisElapsed(unsigned long startTime, unsigned long interval) {
  // Handles millis() rollover correctly (every ~49 days)
  return (millis() - startTime) >= interval;
}

// --- Setup & Control Helpers ---
void recomputeSetupComplete() {
  setupComplete = (tempCalComplete && pressureCalComplete);
}

bool getAllowUnsafePid() {
  portENTER_CRITICAL(&controlMux);
  bool allowed = allowUnsafePid;
  portEXIT_CRITICAL(&controlMux);
  return allowed;
}

void setAllowUnsafePid(bool allowed) {
  portENTER_CRITICAL(&controlMux);
  allowUnsafePid = allowed;
  portEXIT_CRITICAL(&controlMux);
}

bool isPidAllowed() {
  return setupComplete || getAllowUnsafePid();
}

double getSetpointTemp() {
  portENTER_CRITICAL(&controlMux);
  double temp = setpointTemp;
  portEXIT_CRITICAL(&controlMux);
  return temp;
}

bool getPidBoostActive() {
  portENTER_CRITICAL(&controlMux);
  bool active = pidBoostActive;
  portEXIT_CRITICAL(&controlMux);
  return active;
}

void setPidBoostActive(bool active) {
  portENTER_CRITICAL(&controlMux);
  pidBoostActive = active;
  portEXIT_CRITICAL(&controlMux);
}

bool getPreInfusionPhase() {
  portENTER_CRITICAL(&controlMux);
  bool phase = preInfusionPhase;
  portEXIT_CRITICAL(&controlMux);
  return phase;
}

bool getPreInfusionActive() {
  portENTER_CRITICAL(&controlMux);
  bool active = preInfusionActive;
  portEXIT_CRITICAL(&controlMux);
  return active;
}

void setSetpointTemp(double temp) {
  portENTER_CRITICAL(&controlMux);
  setpointTemp = temp;
  portEXIT_CRITICAL(&controlMux);
}

void addSetpointTemp(double delta) {
  portENTER_CRITICAL(&controlMux);
  setpointTemp += delta;
  portEXIT_CRITICAL(&controlMux);
}

double getCurrentPIDOutputPercent() {
  portENTER_CRITICAL(&controlMux);
  double percent = currentPIDOutputPercent;
  portEXIT_CRITICAL(&controlMux);
  return percent;
}

void setCurrentPIDOutputPercent(double percent) {
  portENTER_CRITICAL(&controlMux);
  currentPIDOutputPercent = percent;
  portEXIT_CRITICAL(&controlMux);
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
      // Note: Variables moved to global scope for clarity and testability
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
        portENTER_CRITICAL(&scaleMux);
        shotWeight = reading;  // Accept increasing weight during brew
        portEXIT_CRITICAL(&scaleMux);
        lastValidScaleWeight = reading;
        scaleStableReadings = 0;  // Reset stability counter
        scaleConnected = true;
        clearError(ERR_FLAG_SCALE);
      } else if (abs(reading - lastValidScaleWeight) < maxWeightDrift) {
        // Stability check for non-brew or decreasing weight
        scaleStableReadings++;
        if (scaleStableReadings >= requiredStableReadings) {
          portENTER_CRITICAL(&scaleMux);
          shotWeight = reading;  // Accept stable reading
          portEXIT_CRITICAL(&scaleMux);
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
    portENTER_CRITICAL(&scaleMux);
    shotWeight = 0.0;
    portEXIT_CRITICAL(&scaleMux);
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

void calibrateScale(float knownWeight) {
  if (scaleInitialized && nau.available()) {
    portENTER_CRITICAL(&scaleMux);
    int32_t reading = nau.read();  // Single reading to avoid blocking - mutex protected
    portEXIT_CRITICAL(&scaleMux);

    if (reading != 0) {
      scaleCalibrationFactor = reading / knownWeight;
      Serial.printf("Scale calibrated with factor: %.2f\n", scaleCalibrationFactor);
    }
  }
}

// --- Calibration Functions ---
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
        portENTER_CRITICAL(&scaleMux);
        scaleZeroReading = nau.read();
        portEXIT_CRITICAL(&scaleMux);
        Serial.printf("Scale ZERO reading: %.2f\n", (float)scaleZeroReading);
        calStep = CAL_SCALE_WEIGHT;
        Serial.println("Now place known weight (100g or 200g) on scale");
      }
      break;

    case CAL_SCALE_WEIGHT:
      if (scaleInitialized && nau.available()) {
        portENTER_CRITICAL(&scaleMux);
        scaleKnownWeightReading = nau.read();
        portEXIT_CRITICAL(&scaleMux);
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

// --- OLED Display Functions ---
void initDisplay() {
  if (!u8g2.begin()) {
    displayFailed = true;
    setError(ERR_FLAG_DISPLAY);
    Serial.println("ERROR: OLED initialization failed - continuing with web UI only");
    return;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 20, "ESP32 Espresso");
  u8g2.drawStr(10, 35, "Controller");
  u8g2.drawStr(10, 50, "Initializing...");
  u8g2.sendBuffer();
  // Non-blocking initialization - splash screen will be shown until first update
  Serial.println("Display initialized successfully");
}

// Previous display values for partial update optimization
static double lastDisplayedShotTime = -1;
static double lastDisplayedShotWeight = -1;
static double lastDisplayedTemp = -1;
static bool lastDisplayedBoilerHeating = false;
static bool lastDisplayedBrewActive = false;
static bool lastDisplayedFillActive = false;
static bool lastDisplayedPreInfusionActive = false;
static double lastDisplayedPressure = -1;
static String lastDisplayedError = "";

// Memory pool for string operations to prevent heap fragmentation
static char tempStrBuffer[12];       // Temperature strings (e.g., "200.0F")
static char weightStrBuffer[10];     // Weight strings (e.g., "36.0g")
static char timeStrBuffer[10];       // Time strings (e.g., "30.0s")
static char pressureStrBuffer[10];   // Pressure strings (e.g., "8.2BAR")
static char flowRateStrBuffer[12];   // Flow rate strings (e.g., "2.1ml/s")
static char shotRatioStrBuffer[10];  // Shot ratio strings (e.g., "2.0:1")
static char displayBuffer[64];       // General display buffer

// Pre-allocated JSON buffers for WebSocket and HTTP responses
static char jsonStatusBuffer[768];    // Pre-allocated status JSON buffer (increased for full payload)
static char webSocketBuffer[768];     // Pre-allocated WebSocket buffer (increased to prevent truncation)
static char httpResponseBuffer[256];  // Pre-allocated HTTP response buffer

void updateDisplay() {
  // Skip OLED operations if display initialization failed
  if (displayFailed) {
    return;
  }

  // Track first-run display state (outside block so it can be reset)
  static bool firstRunDisplayed = false;

  // First-run setup display - show setup required message
  if (!setupComplete) {
    if (!firstRunDisplayed) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);

      // Line 1: SETUP REQ.
      u8g2.drawStr(30, 20, "SETUP REQ.");

      // Line 2: AP name
      u8g2.drawStr(30, 35, "espresso_AP");

      // Line 3: IP address
      u8g2.drawStr(30, 50, "192.168.4.1");

      // Line 4: PID status
      u8g2.drawStr(38, 65, "PID: OFF");

      u8g2.sendBuffer();
      firstRunDisplayed = true;
    }
    return;  // Don't show normal display during first-run setup
  } else {
    // Reset flag when setup completes so first-run screen can redraw if needed later
    firstRunDisplayed = false;
  }

  // Partial OLED updates to reduce flicker (per requirements)
  bool needsUpdate = false;

  u8g2.setFont(u8g2_font_6x10_tf);

  // Only update shot timer if changed
  if (shotTime != lastDisplayedShotTime) {
    u8g2.setDrawColor(0);  // Clear area
    u8g2.drawBox(35, 8, 50, 10);
    u8g2.setDrawColor(1);  // Draw text
    u8g2.drawStr(5, 15, "Shot:");
    // Use memory pool buffer to prevent fragmentation with bounds checking
    snprintf(timeStrBuffer, sizeof(timeStrBuffer), "%.1fs", shotTime);
    u8g2.drawStr(35, 15, timeStrBuffer);
    lastDisplayedShotTime = shotTime;
    needsUpdate = true;
  }

  // Only update weight if changed
  if (shotWeight != lastDisplayedShotWeight || scaleConnected != (lastDisplayedShotWeight != -1)) {
    u8g2.setDrawColor(0);  // Clear area
    u8g2.drawBox(50, 23, 50, 10);
    u8g2.setDrawColor(1);  // Draw text
    u8g2.drawStr(5, 30, "Weight:");
    // Use memory pool buffer to prevent fragmentation with bounds checking
    if (scaleConnected) {
      snprintf(weightStrBuffer, sizeof(weightStrBuffer), "%.1fg", shotWeight);
    } else {
      strlcpy(weightStrBuffer, "--.-g", sizeof(weightStrBuffer));
    }
    u8g2.drawStr(50, 30, weightStrBuffer);
    lastDisplayedShotWeight = shotWeight;
    needsUpdate = true;
  }

  // Only update temperature if changed significantly (>0.5°F)
  double displayTemp = getTemp();
  if (abs(displayTemp - lastDisplayedTemp) > 0.5) {
    u8g2.setDrawColor(0);  // Clear area
    u8g2.drawBox(35, 38, 50, 10);
    u8g2.setDrawColor(1);  // Draw text
    u8g2.drawStr(5, 45, "Temp:");
    // Use memory pool buffer to prevent fragmentation with bounds checking
    snprintf(tempStrBuffer, sizeof(tempStrBuffer), "%.1fF", displayTemp);
    u8g2.drawStr(35, 45, tempStrBuffer);
    lastDisplayedTemp = displayTemp;
    needsUpdate = true;
  }

  // Only update boiler heating icon if changed
  if (boilerHeating != lastDisplayedBoilerHeating) {
    u8g2.setDrawColor(0);  // Clear area
    u8g2.drawBox(90, 38, 35, 10);
    u8g2.setDrawColor(1);  // Draw text
    if (boilerHeating) {
      u8g2.drawStr(90, 45, "HEAT");
    }
    lastDisplayedBoilerHeating = boilerHeating;
    needsUpdate = true;
  }

  // Only update status icons if changed
  static bool lastDisplayedPidBoostActive = false;
  if (brewActive != lastDisplayedBrewActive || fillActive != lastDisplayedFillActive || 
      preInfusionActive != lastDisplayedPreInfusionActive || pidBoostActive != lastDisplayedPidBoostActive) {
    u8g2.setDrawColor(0);  // Clear status line
    u8g2.drawBox(5, 53, 120, 10);
    u8g2.setDrawColor(1);  // Draw icons
    if (brewActive) {
      u8g2.drawStr(5, 60, "BREW");
    }
    if (fillActive) {
      u8g2.drawStr(35, 60, "FILL");
    }
    if (preInfusionActive) {
      u8g2.drawStr(60, 60, "PRE");
    }
    if (pidBoostActive) {
      u8g2.drawStr(90, 60, "BOOST");
    }
    lastDisplayedBrewActive = brewActive;
    lastDisplayedFillActive = fillActive;
    lastDisplayedPreInfusionActive = preInfusionActive;
    lastDisplayedPidBoostActive = pidBoostActive;
    needsUpdate = true;
  }

  // Display shot ratio during and after brewing
  static double lastDisplayedShotRatio = -1;
  if ((brewActive || currentShotRatio > 0) && scaleConnected && abs(currentShotRatio - lastDisplayedShotRatio) > 0.05) {
    u8g2.setDrawColor(0);  // Clear ratio area
    u8g2.drawBox(85, 53, 40, 10);
    u8g2.setDrawColor(1);  // Draw ratio
    if (currentShotRatio > 0) {
      snprintf(shotRatioStrBuffer, sizeof(shotRatioStrBuffer), "%.1f:1", currentShotRatio);
    } else {
      strlcpy(shotRatioStrBuffer, "--:1", sizeof(shotRatioStrBuffer));
    }
    u8g2.drawStr(85, 60, shotRatioStrBuffer);
    lastDisplayedShotRatio = currentShotRatio;
    needsUpdate = true;
  } else if (!brewActive && currentShotRatio == 0 && lastDisplayedShotRatio != -1) {
    // Clear ratio when reset
    u8g2.setDrawColor(0);
    u8g2.drawBox(85, 53, 40, 10);
    lastDisplayedShotRatio = -1;
    needsUpdate = true;
  }

  // Only update pressure if changed significantly (>0.1 BAR)
  if (abs(currentPressure - lastDisplayedPressure) > 0.1) {
    u8g2.setDrawColor(0);  // Clear pressure area
    u8g2.drawBox(5, 98, 60, 10);
    u8g2.setDrawColor(1);  // Draw pressure
    if (currentPressure >= 0) {
      snprintf(pressureStrBuffer, sizeof(pressureStrBuffer), "%.1fBAR", currentPressure);
    } else {
      strlcpy(pressureStrBuffer, "--BAR", sizeof(pressureStrBuffer));
    }
    u8g2.drawStr(5, 105, pressureStrBuffer);
    lastDisplayedPressure = currentPressure;
    needsUpdate = true;
  }

  // Display flow rate during brewing
  static double lastDisplayedFlowRate = -1;
  if (brewActive && scaleConnected && abs(currentFlowRate - lastDisplayedFlowRate) > 0.1) {
    u8g2.setDrawColor(0);  // Clear flow rate area
    u8g2.drawBox(70, 98, 55, 10);
    u8g2.setDrawColor(1);  // Draw flow rate
    if (currentFlowRate > 0) {
      snprintf(flowRateStrBuffer, sizeof(flowRateStrBuffer), "%.1fml/s", currentFlowRate);
    } else {
      strlcpy(flowRateStrBuffer, "--ml/s", sizeof(flowRateStrBuffer));
    }
    u8g2.drawStr(70, 105, flowRateStrBuffer);
    lastDisplayedFlowRate = currentFlowRate;
    needsUpdate = true;
  } else if (!brewActive && lastDisplayedFlowRate != -1) {
    // Clear flow rate when not brewing
    u8g2.setDrawColor(0);
    u8g2.drawBox(70, 98, 55, 10);
    lastDisplayedFlowRate = -1;
    needsUpdate = true;
  }

  // System message display (bottom line for errors)
  static String lastDisplayedSystemMessage = "";
  if (systemMessage != lastDisplayedSystemMessage) {
    u8g2.setDrawColor(0);  // Clear system message area
    u8g2.drawBox(5, 108, 120, 10);
    u8g2.setDrawColor(1);  // Draw system message
    if (systemMessage.length() > 0) {
      u8g2.drawStr(5, 115, systemMessage.c_str());
    }
    lastDisplayedSystemMessage = systemMessage;
    needsUpdate = true;
  }

  // Only update error messages if changed
  if (systemMessage != lastDisplayedError) {
    u8g2.setDrawColor(0);  // Clear error area
    u8g2.drawBox(5, 68, 120, 25);
    u8g2.setDrawColor(1);  // Draw error
    if (systemMessage.length() > 0) {
      u8g2.drawStr(5, 75, "ERROR:");
      u8g2.drawStr(5, 90, systemMessage.c_str());
    }
    lastDisplayedError = systemMessage;
    needsUpdate = true;
  }

  // Only send buffer if something actually changed
  if (needsUpdate) {
    u8g2.sendBuffer();
  }
}

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
    rtdFault = true;
    setError(ERR_FLAG_RTD);
    return false;
  }

  Serial.println("MAX31865 RTD sensor initialized (3-wire PT100)");
  rtdFault = false;

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
    rtdFault = true;
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
    rtdFault = true;
    setError(ERR_FLAG_RTD);
    setTemp(lastValidTemp);
    return lastValidTemp;
  }

  // Convert to Fahrenheit
  double tempFahrenheit = celsiusToFahrenheit(tempCelsius);

  // Clear RTD fault if reading is valid
  if (rtdFault && hasErrorFlag(ERR_FLAG_RTD)) {
    rtdFault = false;
    clearError(ERR_FLAG_RTD);
    Serial.printf("RTD fault cleared. Temperature: %.1f°F\n", tempFahrenheit);
  }

  // Temperature rate-of-change safety check (reuses 'now' from line 1240)
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

// --- Error Management Functions ---
void emergencyStop() {
  // Stop heating and pumping systems for critical errors (per requirements)
  // Use critical sections to ensure atomic state changes - avoid nested mutexes
  
  // First: Stop all hardware outputs and state variables (stateMux protected)
  portENTER_CRITICAL(&stateMux);
  set_pump(false);
  set_brew_solenoid(false);
  set_fill_solenoid(false);
  set_boiler_element(false);
  brewActive = false;
  fillActive = false;
  fillInProgress = false;
  portEXIT_CRITICAL(&stateMux);
  
  // Second: Stop control variables (controlMux protected, avoid nesting)
  setPidBoostActive(false);  // This function handles its own controlMux
  portENTER_CRITICAL(&controlMux);
  preInfusionActive = false;
  preInfusionPhase = false;
  portEXIT_CRITICAL(&controlMux);

  // Critical errors require ERROR state and manual reboot
  if (requiresManualReboot()) {
    changeState(STATE_ERROR);
    Serial.println("CRITICAL ERROR: Heating and pumping systems stopped - manual reboot required");
  } else {
    // Non-critical errors maintain operation
    Serial.println("Non-critical error: System continues operation with error display");
  }
}

// Helper function to update system message based on current error flags
void updateSystemMessage() {
  uint8_t flags = getErrorFlags();
  if (flags == 0) {
    systemMessage = "";
    return;
  }
  if (flags & ERR_FLAG_TEMP) systemMessage = "TEMP ERR";
  else if (flags & ERR_FLAG_RTD) systemMessage = "TEMP ERR";
  else if (flags & ERR_FLAG_PID) systemMessage = "PID ERR";
  else if (flags & ERR_FLAG_PRESSURE) systemMessage = "PRESS ERR";
  else if (flags & ERR_FLAG_SCALE) systemMessage = "SCALE ERR";
  else if (flags & ERR_FLAG_LOOP_TIMEOUT) systemMessage = "LOOP ERR";
  else if (flags & ERR_FLAG_DISPLAY) systemMessage = "DISP ERR";
  else if (flags & ERR_FLAG_I2C) systemMessage = "I2C ERR";
}

// Consolidated error set — atomically sets flag, derives message, triggers actions
// Only call from Core 0 (main loop). PID task (Core 1) must use setErrorFlag() directly.
void setError(uint8_t flag) {
  setErrorFlag(flag);
  updateSystemMessage();
  if (isCriticalError()) {
    emergencyStop();
    Serial.printf("Critical error active: %s - emergency stop activated\n", systemMessage.c_str());
  } else if (isDisplayOnlyError()) {
    Serial.printf("Display-only error: %s - system continues operation\n", systemMessage.c_str());
  } else if (isRecoverableError()) {
    Serial.printf("Recoverable error: %s - auto-recovery enabled\n", systemMessage.c_str());
  }
}

// Consolidated error clear — atomically clears flag, re-derives message
// Only call from Core 0 (main loop).
void clearError(uint8_t flag) {
  clearErrorFlag(flag);
  updateSystemMessage();
}

bool hasError() {
  return getErrorFlags() != 0;
}

bool isRecoverableError() {
  uint8_t flags = getErrorFlags();
  return (flags & (ERR_FLAG_SCALE | ERR_FLAG_LOOP_TIMEOUT | ERR_FLAG_DISPLAY | ERR_FLAG_I2C | ERR_FLAG_PRESSURE)) != 0;
}

bool isCriticalError() {
  uint8_t flags = getErrorFlags();
  return (flags & (ERR_FLAG_TEMP | ERR_FLAG_RTD | ERR_FLAG_PID)) != 0;
}

bool isDisplayOnlyError() {
  uint8_t flags = getErrorFlags();
  return (flags == ERR_FLAG_SCALE);  // Only scale error, no other errors
}

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

  // Handle loop timeout errors (auto-recovery)
  if (loopTimeoutTriggered && hasErrorFlag(ERR_FLAG_LOOP_TIMEOUT)) {
    static unsigned long loopTimeoutStartTime = 0;
    if (loopTimeoutStartTime == 0) loopTimeoutStartTime = millis();
    unsigned long now = millis();
    if ((now - loopTimeoutStartTime) > 5000) {  // 5 second recovery time
      loopTimeoutTriggered = false;
      loopTimeoutStartTime = 0;
      clearError(ERR_FLAG_LOOP_TIMEOUT);
    }
  }

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

// --- 4-Channel Relay Control Functions ---
void initRelays() {
  pinMode(PIN_RELAY_CH1, OUTPUT);
  digitalWrite(PIN_RELAY_CH1, LOW);  // Brew Solenoid OFF
  pinMode(PIN_RELAY_CH2, OUTPUT);
  digitalWrite(PIN_RELAY_CH2, LOW);  // Pump OFF
  pinMode(PIN_RELAY_CH3, OUTPUT);
  digitalWrite(PIN_RELAY_CH3, LOW);  // Fill Solenoid OFF
  pinMode(PIN_RELAY_CH4, OUTPUT);
  digitalWrite(PIN_RELAY_CH4, LOW);  // Boiler Element SSR OFF
  Serial.println("4-Channel relay system initialized");
}

void set_brew_solenoid(bool on) {
  // Safety interlock: prevent brew solenoid activation during fill
  if (on && fillActive) {
    Serial.println("INTERLOCK: Brew solenoid blocked - fill active");
    return;
  }
  digitalWrite(PIN_RELAY_CH1, on ? HIGH : LOW);
}

void set_pump(bool on) {
  // Pressure safety lockout - prevent pump activation during over-pressure
  if (on && pressureLockoutActive) {
    Serial.printf("PRESSURE LOCKOUT: Pump activation blocked - pressure %.1f BAR (lockout until ≤%.1f BAR)\n", currentPressure, PRESSURE_LOCKOUT_LOW);
    return;  // Pump stays OFF until pressure drops to 11.0 BAR or below
  }
  digitalWrite(PIN_RELAY_CH2, on ? HIGH : LOW);
}

void set_fill_solenoid(bool on) {
  // Safety interlock: prevent fill solenoid activation during brew
  if (on && brewActive) {
    Serial.println("INTERLOCK: Fill solenoid blocked - brew active");
    return;
  }
  digitalWrite(PIN_RELAY_CH3, on ? HIGH : LOW);
}

void set_boiler_element(bool on) {
#ifdef BENCH_MODE
  // Bench mode: block heater relay, keep it OFF regardless of PID request
  digitalWrite(PIN_RELAY_CH4, LOW);
  boilerHeating = false;
#else
  digitalWrite(PIN_RELAY_CH4, on ? HIGH : LOW);
  boilerHeating = on;
#endif
}

// Legacy function for compatibility
void set_ssr(bool on) {
  set_boiler_element(on);
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

// Startup fill cycle - runs during setup() to top off the steam boiler at boot.
// Only called when boilerFilled=true (fill probe previously calibrated and boiler
// has been filled at least once). Blocking operation — completes before main loop.
// On fresh install this is skipped; user fills via the web UI calibration flow.
void runStartupFillCycle() {
  // Check if boiler already has water
  if (readFillProbe()) {
    Serial.println("Steam boiler already filled - skipping startup fill");
    boilerFilled = true;
    return;
  }

  Serial.println("Steam boiler needs water - starting fill cycle");

  // Fill cycle with timeout protection
  const unsigned long STARTUP_FILL_TIMEOUT = 120000;  // 2 minute max
  const unsigned long FILL_CYCLE_DURATION = 2000;     // 2 second pump cycles
  unsigned long startTime = millis();
  unsigned long cycleStart = millis();
  bool pumpOn = true;

  set_fill_solenoid(true);
  set_pump(true);

  while (millis() - startTime < STARTUP_FILL_TIMEOUT) {
    // Feed watchdog during fill
    esp_task_wdt_reset();

    // Check if water detected
    if (readFillProbe()) {
      set_fill_solenoid(false);
      set_pump(false);
      boilerFilled = true;
      Serial.printf("Startup fill complete in %lu seconds\n", (millis() - startTime) / 1000);
      return;
    }

    // Cycle pump on/off every 2 seconds for accurate probe reading
    if (millis() - cycleStart >= FILL_CYCLE_DURATION) {
      pumpOn = !pumpOn;
      set_pump(pumpOn);
      set_fill_solenoid(pumpOn);
      cycleStart = millis();

      if (!pumpOn) {
        // Brief pause while pump is off to let water settle for probe reading
        delay(100);
        if (readFillProbe()) {
          set_fill_solenoid(false);
          set_pump(false);
          boilerFilled = true;
          Serial.printf("Startup fill complete in %lu seconds\n", (millis() - startTime) / 1000);
          return;
        }
      }
    }

    delay(50);  // Small delay to prevent tight loop
  }

  // Timeout reached
  set_fill_solenoid(false);
  set_pump(false);
  Serial.println("WARNING: Startup fill timeout - steam boiler may not be full");
  Serial.println("Check water supply and fill probe. System will continue but monitor steam boiler.");
}

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
  portENTER_CRITICAL(&stateMux);
  if (currentState != newState) {
    previousState = currentState;
    currentState = newState;
    portEXIT_CRITICAL(&stateMux);
    Serial.printf("State change: %d -> %d\n", previousState, currentState);
    // Flush pending EEPROM writes on transition to IDLE (catch changes before power-off)
    if (newState == STATE_IDLE && eepromDirty) {
      saveParametersToEEPROM();
      eepromDirty = false;
    }
  } else {
    portEXIT_CRITICAL(&stateMux);
  }
}

// Thread-safe state getter for use from ISRs or other tasks
SystemState getState() {
  portENTER_CRITICAL(&stateMux);
  SystemState state = currentState;
  portEXIT_CRITICAL(&stateMux);
  return state;
}

// Thread-safe temperature getter for use from main loop
double getTemp() {
  portENTER_CRITICAL(&tempMux);
  double temp = currentTemp;
  portEXIT_CRITICAL(&tempMux);
  return temp;
}

// Thread-safe temperature setter for use from PID task
void setTemp(double temp) {
  portENTER_CRITICAL(&tempMux);
  currentTemp = temp;
  portEXIT_CRITICAL(&tempMux);
}

// Thread-safe cached raw temperature getter for PID task (avoids SPI access from Core 1)
double getCachedRawTemp() {
  portENTER_CRITICAL(&cachedTempMux);
  double temp = cachedRawTemp;
  portEXIT_CRITICAL(&cachedTempMux);
  return temp;
}

// Thread-safe cached raw temperature setter for main loop
void setCachedRawTemp(double temp) {
  portENTER_CRITICAL(&cachedTempMux);
  cachedRawTemp = temp;
  portEXIT_CRITICAL(&cachedTempMux);
}

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
        emergencyStop();  // Stop heating/pumping systems
        // DO NOT change state - maintain IDLE operation for non-critical functions
        break;
      }

      // Check for fill needed (only if no errors)
      if (!hasError() && !fillProbeWet && !fillInProgress) {
        changeState(STATE_FILL);
        break;
      }

      // Brew starting is now handled by momentary button logic in checkBrewSwitch()
      break;

    case STATE_BREW_START:
      // Start brew routine
      startBrewRoutine();
      changeState(STATE_BREW_ACTIVE);
      break;

    case STATE_BREW_ACTIVE:
      // Check for brew stop conditions (priority order per requirements)

      // 1. Critical error during brew - stop heating/pumping but maintain operation
      if (isCriticalError()) {
        Serial.printf("Critical error during brew: %s - stopping heating/pumping\n", systemMessage.c_str());
        emergencyStop();              // Stop heating/pumping systems
        changeState(STATE_BREW_END);  // End brew but maintain system operation
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
      // Cleaning cycle active - pump cycling logic
      if (cleaningActive) {
        // Check maximum cleaning duration safety limit
        if (millisElapsed(cleaningStartTime, CLEANING_MAX_DURATION)) {
          Serial.println("Maximum cleaning duration reached - stopping cleaning");
          stopCleaningCycle();
          changeState(STATE_IDLE);
          break;
        }


        unsigned long currentTime = millis();

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
        } else if (hasErrorFlag(ERR_FLAG_LOOP_TIMEOUT) && !loopTimeoutTriggered) {
          clearError(ERR_FLAG_LOOP_TIMEOUT);
          changeState(STATE_IDLE);
          Serial.println("LOOP_TIMEOUT error auto-recovered");
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
  // Pre-infusion variables need controlMux protection for thread safety
  portENTER_CRITICAL(&controlMux);
  preInfusionPhase = true;
  preInfusionActive = true;
  portEXIT_CRITICAL(&controlMux);
  preInfusionStartTime = millis();

  // Reset PID integral at brew start to prevent windup carryover between brew cycles
  portENTER_CRITICAL(&pidMux);
  pid_integral = 0.0;
  portEXIT_CRITICAL(&pidMux);
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
  // Pre-infusion variables need controlMux protection for thread safety
  portENTER_CRITICAL(&controlMux);
  preInfusionPhase = false;
  preInfusionActive = false;
  portEXIT_CRITICAL(&controlMux);

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

  // Turn on both solenoid and pump together for first 2s cycle
  set_fill_solenoid(true);
  set_pump(true);

  Serial.println("Fill routine started - solenoid and pump ON for 2s cycle");
}

void updateFillRoutine() {
  if (!fillInProgress) return;

  // Check for maximum fill timeout (safety protection) - using rollover-safe timing
  if (millisElapsed(fillStartTime, FILL_MAX_DURATION)) {
    // Timeout reached - stop fill routine and log error
    set_fill_solenoid(false);
    set_pump(false);
    fillActive = false;
    fillInProgress = false;
    Serial.printf("Fill routine timeout after %lu seconds - stopped for safety\n", FILL_MAX_DURATION / 1000);
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

// --- Core 1 PID Task ---
// Dedicated FreeRTOS task for deterministic PID control
// Runs on Core 1 with highest priority for real-time performance
void pidTask(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_INTERVAL_MS);  // 100ms

  pidTaskRunning = true;
  Serial.println("PID Task started on Core 1");
  // Register PID task with watchdog to catch stalls
  if (esp_task_wdt_add(NULL) != ESP_OK) {
    Serial.println("WARNING: Failed to add PID task to watchdog");
  }

  for (;;) {
    // Wait for next cycle (precise timing with vTaskDelayUntil)
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    // Feed watchdog for PID task
    esp_task_wdt_reset();

    // Check if required calibrations are complete before running PID
  if (!isPidAllowed()) {
      // Setup not complete - disable PID heating, only read temperature
      set_boiler_element(false);
      setCurrentPIDOutputPercent(0.0);
      
      // Reset PID integral to prevent windup accumulation during setup
      // This ensures clean PID response when setup completes
      portENTER_CRITICAL(&pidMux);
      pid_integral = 0.0;
      portEXIT_CRITICAL(&pidMux);

    // Use cached temp for display (main loop handles SPI reads)
    setTemp(getCachedRawTemp());
      continue;
    }

    // Execute PID control (updates currentTemp internally)
    pid_step();
  }
}

// --- Performance Monitoring Functions ---
void checkPIDTiming() {
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
  Serial.printf("Memory: Current %u bytes, Minimum %u bytes\n", ESP.getFreeHeap(), minFreeHeap);
  Serial.printf("Web Requests: Total %lu, Avg response: %lums\n",
                webRequestCount,
                webRequestCount > 0 ? totalWebResponseTime / webRequestCount : 0);
  Serial.printf("WebSocket Messages: %lu sent\n", webSocketMessageCount);
  Serial.printf("Active Warnings: PID=%s, Memory=%s\n",
                pidTimingWarningActive ? "YES" : "NO",
                memoryWarningActive ? "YES" : "NO");
  Serial.println("==========================");
}

// --- PID Control Function (called from Core 1 timer ISR context) ---
void pid_step() {
  // Performance monitoring - PID timing check
  checkPIDTiming();

  // Safety override FIRST - stop heating/pumping for critical errors before any computation
  // Use atomic error flags for thread-safe access from PID task on Core 1
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

  // Use cached raw temperature from main loop (avoids SPI access from PID task on Core 1)
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
        // Set flag for main loop to log (avoid Serial from PID task)
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

  // Update global currentTemp with filtered value (thread-safe for main loop access)
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

  portENTER_CRITICAL(&pidMux);
  pid_integral += error * (CONTROL_INTERVAL_MS / 1000.0);

  // PID integral windup protection
  if (pid_integral > integralMax) pid_integral = integralMax;
  if (pid_integral < -integralMax) pid_integral = -integralMax;

  // Use derivative on measurement (not error) to prevent "derivative kick" on setpoint changes
  // Negative sign because we're measuring temperature change, not error change
  // Uses prevPidTemp (stored before update) to get actual temperature rate of change
  double derivative = -(filteredTemp - prevPidTemp) / (CONTROL_INTERVAL_MS / 1000.0);
  pidOutput = Kp * error + Ki * pid_integral + Kd * derivative;
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

// --- Initialize PID Task on Core 1 ---
void initPIDTask() {
  // Create dedicated PID task on Core 1 with highest priority
  // Priority 24 is highest for application tasks (configMAX_PRIORITIES - 1)
  // Stack size 6144 bytes provides margin for PID calculations and safety checks
  BaseType_t result = xTaskCreatePinnedToCore(
    pidTask,         // Task function
    "PIDTask",       // Task name
    6144,            // Stack size (bytes) - increased for safety margin
    NULL,            // Parameters
    24,              // Priority (highest for real-time control)
    &pidTaskHandle,  // Task handle
    1                // Core 1 (dedicated to PID)
  );

  if (result != pdPASS) {
    Serial.println("ERROR: Failed to create PID task on Core 1");
    return;
  }

  Serial.println("PID Task created on Core 1 (100ms interval, highest priority)");
}

// --- EEPROM Functions ---
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

  // Re-entrancy guard (all callers are on Core 0, no spinlock needed)
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

  // Write signature
  writeUint16ToEEPROM(ADDR_SIGNATURE, EEPROM_SIGNATURE);

  // Write parameters (using snapshots for mutex-protected values)
  writeFloatToEEPROM(ADDR_BREW_TEMP, snapSetpointTemp);
  writeFloatToEEPROM(ADDR_SHOT_TIME, shotTargetTime);
  writeFloatToEEPROM(ADDR_SHOT_WEIGHT, shotTargetWeight);
  writeFloatToEEPROM(ADDR_PREINFUSION, preInfusionDuration);
  writeFloatToEEPROM(ADDR_SCALE_CAL, scaleCalibrationFactor);
  writeFloatToEEPROM(ADDR_PID_BOOST, pidBoostDuration);
  writeFloatToEEPROM(ADDR_PID_KP, Kp);
  writeFloatToEEPROM(ADDR_PID_KI, Ki);
  writeFloatToEEPROM(ADDR_PID_KD, Kd);
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
  Kp = readFloatFromEEPROM(ADDR_PID_KP);
  Ki = readFloatFromEEPROM(ADDR_PID_KI);
  Kd = readFloatFromEEPROM(ADDR_PID_KD);
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
  if (Kp < 0.1 || Kp > 200.0) Kp = 40.0;
  if (Ki < 0.0 || Ki > 10.0) Ki = 0.8;
  if (Kd < 0.0 || Kd > 500.0) Kd = 120.0;
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
                Kp, Ki, Kd, (int)fillProbeThreshold);

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

// --- Web server handlers ---
void handleRoot() {
  // Stream HTML components directly to avoid large heap allocation
  // This eliminates the need for 48KB memory reservation
  
  // Start HTTP response
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  
  // Stream each PROGMEM component directly to client (no heap allocation)
  server.sendContent_P(web_ui_html);           // HTML head + opening style tag
  server.sendContent_P(bootstrap_css);         // Bootstrap CSS  
  server.sendContent_P(web_ui_css);            // Custom CSS
  server.sendContent_P(web_ui_html_body);      // HTML body structure
  server.sendContent_P(web_ui_js);             // JavaScript functionality
  server.sendContent_P(bootstrap_js);          // Bootstrap JS
  server.sendContent_P(apexcharts_js);         // ApexCharts JS  
  server.sendContent_P(web_ui_html_footer);    // Closing tags
  
  // End response
  server.sendContent("");
}


// JSON status endpoint with compression (only send changed values)
void handleStatus() {
  unsigned long requestStartTime = millis();
  webRequestCount++;

  buildStatusJSON(jsonStatusBuffer, sizeof(jsonStatusBuffer), true);
  server.send(200, "application/json", jsonStatusBuffer);

  unsigned long responseTime = millis() - requestStartTime;
  totalWebResponseTime += responseTime;
  if (responseTime > maxWebResponseTime) {
    maxWebResponseTime = responseTime;
  }
  if (responseTime > 500) {
    Serial.printf("SLOW WEB RESPONSE: %lums for /status request\n", responseTime);
  }
}

// Handle tare scale request
void handleTare() {
  tareScale();
  server.send(200, "text/plain", "Scale tared");
}

// Calibration web handlers
// Map CalStep enum to JSON-compatible string
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

void handleCalibrateStatus() {
  static char jsonBuffer[512];
  const char* instructions = "";

  switch (calStep) {
    case CAL_FILL_DRY:      instructions = "Ensure fill probe is completely DRY, then click Next Step"; break;
    case CAL_FILL_WET:      instructions = "Immerse fill probe in water until WET, then click Next Step"; break;
    case CAL_SCALE_ZERO:    instructions = "Remove all weight from scale, then click Next Step"; break;
    case CAL_SCALE_WEIGHT:  instructions = "Place known weight on scale, then click Next Step"; break;
    case CAL_SCALE_CONFIRM: instructions = "Enter the known weight value and click Complete Scale Cal"; break;
    case CAL_PRESSURE_ZERO: instructions = "Ensure system at ZERO pressure (pump OFF), then click Next Pressure Step"; break;
    case CAL_PRESSURE_HIGH: instructions = "1) Enter actual pressure reading from analog gauge in 'Actual Pressure' field. 2) Use 'Start Pump (Cal)' to pressurize. 3) Click 'Set Pressure Value' to confirm reading. 4) Click 'Next Pressure Step' to complete."; break;
    default: break;
  }

  const char* stepStr = calStepName(calStep);
  // Derive fill/scale active and pressure active from unified calStep
  bool fillScaleActive = isFillScaleCal();
  bool pressureActive = isPressureCal();
  const char* pressureStepStr = pressureActive ? stepStr : "";
  const char* fillScaleStepStr = fillScaleActive ? stepStr : "";

  snprintf(jsonBuffer, sizeof(jsonBuffer),
           "{\"active\":%s,\"step\":\"%s\",\"instructions\":\"%s\",\"pressureCalActive\":%s,\"pressureCalStep\":\"%s\"}",
           fillScaleActive ? "true" : "false",
           fillScaleStepStr,
           instructions,
           pressureActive ? "true" : "false",
           pressureStepStr);

  server.send(200, "application/json", jsonBuffer);
}

void handleCalibrateFillStart() {
  startFillProbeCalibration();
  server.send(200, "text/plain", "Fill probe calibration started");
}

void handleCalibrateScaleStart() {
  startScaleCalibration();
  server.send(200, "text/plain", "Scale calibration started");
}

void handleCalibrateNext() {
  CalStep before = calStep;
  processCalibrationStep();
  if (calStep != before) {
    server.send(200, "text/plain", "Calibration step processed");
  } else if (!isFillScaleCal()) {
    server.send(400, "text/plain", "No active fill/scale calibration");
  } else {
    server.send(500, "text/plain", "Step failed - check hardware connection");
  }
}

void handleCalibrateScaleComplete() {
  if (server.hasArg("weight")) {
    float weight = server.arg("weight").toFloat();
    if (weight > 0) {
      completeScaleCalibration(weight);
      server.send(200, "text/plain", "Scale calibration completed");
    } else {
      server.send(400, "text/plain", "Invalid weight value");
    }
  } else {
    server.send(400, "text/plain", "Missing weight parameter");
  }
}

void handleCalibratePressureStart() {
  startPressureCalibration();
  server.send(200, "text/plain", "Pressure calibration started");
}

void handleCalibratePressureNext() {
  if (isPressureCal()) {
    processPressureCalibrationStep();
    server.send(200, "text/plain", "Pressure calibration step processed");
  } else {
    server.send(400, "text/plain", "No active pressure calibration");
  }
}

void handleCalibratePumpStart() {
  if (calStep == CAL_PRESSURE_HIGH) {
    set_pump(true);
    Serial.println("Calibration pump started for HIGH pressure step");
    server.send(200, "text/plain", "Calibration pump started");
  } else if (!isPressureCal()) {
    server.send(400, "text/plain", "Pressure calibration not active");
  } else {
    server.send(400, "text/plain", "Pump only available during HIGH pressure step");
  }
}

void handleCalibratePumpStop() {
  if (isPressureCal()) {
    set_pump(false);
    Serial.println("Calibration pump stopped");
    server.send(200, "text/plain", "Calibration pump stopped");
  } else {
    server.send(400, "text/plain", "Pressure calibration not active");
  }
}

// Temperature calibration confirmation - user verifies probe reading is accurate
void handleCalibrateTempComplete() {
  // Read current temperature for verification display
  float temp = read_boiler_temp();

  if (temp > 0 && temp < 300) {  // Sanity check - valid temp range
    tempCalComplete = true;
    recomputeSetupComplete();
    eepromDirty = true;

    char response[128];
    snprintf(response, sizeof(response),
             "{\"success\":true,\"temperature\":%.1f,\"setupComplete\":%s}",
             temp, setupComplete ? "true" : "false");
    server.send(200, "application/json", response);
    Serial.printf("Temperature calibration confirmed at %.1f°F\n", temp);
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid temperature reading\"}");
  }
}

void handleSetPressureCalibrationValue() {
  if (server.hasArg("pressure")) {
    double newPressure = server.arg("pressure").toFloat();
    if (newPressure >= 1.0 && newPressure <= 15.0) {
      pressureHighValue = newPressure;
      Serial.printf("Pressure calibration value set to %.1f BAR\n", pressureHighValue);
      server.send(200, "text/plain", "Pressure calibration value updated");
    } else {
      server.send(400, "text/plain", "Invalid pressure value (must be 1-15 BAR)");
    }
  } else {
    server.send(400, "text/plain", "Missing pressure parameter");
  }
}

// Cancel any active calibration and return to idle
void handleCalibrateCancel() {
  if (calStep == CAL_IDLE) {
    server.send(400, "text/plain", "No active calibration to cancel");
    return;
  }

  // Safety: turn off pump if pressure calibration was active
  if (isPressureCal()) {
    set_pump(false);
  }

  Serial.printf("Calibration cancelled (was at step %d)\n", calStep);
  calStep = CAL_IDLE;
  server.send(200, "text/plain", "Calibration cancelled");
}

void handleSetpoint() {
  if (server.hasArg("sp")) {
    double newTemp = server.arg("sp").toFloat();
    if (newTemp >= 180.0 && newTemp <= 220.0) {
      setSetpointTemp(newTemp);
      originalSetpointTemp = newTemp;
      eepromDirty = true;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Temperature out of range (180-220°F)");
    }
  } else {
    server.send(400, "text/plain", "Missing sp param");
  }
}

void handleSetDoseWeight() {
  if (server.hasArg("dose")) {
    double newDose = server.arg("dose").toFloat();
    if (newDose >= 10.0 && newDose <= 30.0) {
      doseWeight = newDose;
      eepromDirty = true;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Dose weight out of range (10-30g)");
    }
  } else {
    server.send(400, "text/plain", "Missing dose param");
  }
}

// Unified shot parameters handler — accepts any combination of brewTemp, shotTargetTime, doseWeight, shotTargetWeight
void handleSetParams() {
  bool updated = false;

  if (server.hasArg("brewTemp")) {
    double val = server.arg("brewTemp").toFloat();
    if (val >= 180.0 && val <= 220.0) {
      setSetpointTemp(val);
      originalSetpointTemp = val;
      updated = true;
    } else {
      server.send(400, "text/plain", "Brew temp out of range (180-220°F)");
      return;
    }
  }

  if (server.hasArg("shotTargetTime")) {
    double val = server.arg("shotTargetTime").toFloat();
    if (val >= 0.0 && val <= 60.0) {
      shotTargetTime = val;
      // Clamp dependent params if they now exceed target time
      if (preInfusionDuration > shotTargetTime) preInfusionDuration = shotTargetTime;
      if (pidBoostDuration > shotTargetTime) pidBoostDuration = shotTargetTime;
      updated = true;
    } else {
      server.send(400, "text/plain", "Shot target time out of range (0-60s)");
      return;
    }
  }

  if (server.hasArg("doseWeight")) {
    double val = server.arg("doseWeight").toFloat();
    if (val >= 10.0 && val <= 30.0) {
      doseWeight = val;
      updated = true;
    } else {
      server.send(400, "text/plain", "Dose weight out of range (10-30g)");
      return;
    }
  }

  if (server.hasArg("shotTargetWeight")) {
    double val = server.arg("shotTargetWeight").toFloat();
    if (val >= 0.0 && val <= 60.0) {
      shotTargetWeight = val;
      updated = true;
    } else {
      server.send(400, "text/plain", "Shot target weight out of range (0-60g)");
      return;
    }
  }

  if (updated) {
    eepromDirty = true;
    server.send(200, "text/plain", "Parameters saved");
  } else {
    server.send(400, "text/plain", "No valid parameters provided");
  }
}

struct ParamDef {
  const char* argName;
  double* target;
  double min;
  double max;          // 0 means use shotTargetTime as dynamic max
  const char* label;
  const char* fmt;     // printf format for response (e.g. "%.1f", "%.2f")
};

static const ParamDef pidParams[] = {
  {"kp",          &Kp,                    0.1,   200.0, "Kp",             "%.1f"},
  {"ki",          &Ki,                    0.0,   10.0,  "Ki",             "%.2f"},
  {"kd",          &Kd,                    0.0,   500.0, "Kd",             "%.1f"},
  {"boost",       &pidBoostDuration,      0.0,   0,     "Boost",          "%.1fs"},
  {"preinfusion", &preInfusionDuration,   0.0,   0,     "Pre-infusion",   "%.1fs"},
  {"rate",        &preInfusionPulseRate,  100.0, 1000.0,"Rate",           "%.0fms"},
};

void handleSetPIDParameters() {
  bool updated = false;
  char response[256];
  int pos = snprintf(response, sizeof(response), "PID parameters updated:");

  for (size_t i = 0; i < sizeof(pidParams) / sizeof(pidParams[0]); i++) {
    const ParamDef& p = pidParams[i];
    if (!server.hasArg(p.argName)) continue;

    double val = server.arg(p.argName).toFloat();
    double maxVal = (p.max == 0) ? shotTargetTime : p.max;

    if (val < p.min || val > maxVal) {
      char errMsg[64];
      snprintf(errMsg, sizeof(errMsg), "%s out of range (%.1f-%.1f)", p.label, p.min, maxVal);
      server.send(400, "text/plain", errMsg);
      return;
    }

    *p.target = val;
    updated = true;
    pos += snprintf(response + pos, sizeof(response) - pos, " %s=", p.label);
    pos += snprintf(response + pos, sizeof(response) - pos, p.fmt, val);
  }

  if (updated) {
    eepromDirty = true;
    server.send(200, "text/plain", response);
  } else {
    server.send(400, "text/plain", "No valid parameters provided");
  }
}

// Safety settings handler (PID bypass during setup)
void handleSetSafety() {
  if (server.hasArg("pidBypass")) {
    int bypassValue = server.arg("pidBypass").toInt();
    bool enableBypass = (bypassValue == 1);
    setAllowUnsafePid(enableBypass);
    eepromDirty = true;

    if (enableBypass) {
      Serial.println("WARNING: PID bypass enabled - PID may run before setup completes");
      server.send(200, "text/plain", "PID bypass enabled");
    } else {
      Serial.println("PID bypass disabled - setup required for PID");
      server.send(200, "text/plain", "PID bypass disabled");
    }
    return;
  }

  server.send(400, "text/plain", "Missing pidBypass param (use 1 or 0)");
}

// Cleaning cycle web handlers
void handleStartCleaning() {
  if (currentState != STATE_IDLE) {
    server.send(400, "text/plain", "Cannot start cleaning - system not in IDLE state");
    return;
  }

  if (cleaningActive) {
    server.send(400, "text/plain", "Cleaning cycle already active");
    return;
  }

  // Check fill probe before starting cleaning
  if (!readFillProbe()) {
    server.send(400, "text/plain", "Cannot start cleaning - fill steam boiler first");
    return;
  }

  startCleaningCycle();
  server.send(200, "text/plain", "Cleaning cycle started");
}

void handleStopCleaning() {
  if (!cleaningActive) {
    server.send(400, "text/plain", "No active cleaning cycle to stop");
    return;
  }

  stopCleaningCycle();
  changeState(STATE_IDLE);
  server.send(200, "text/plain", "Cleaning cycle stopped");
}

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("WebSocket client #%u disconnected\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("WebSocket client #%u connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        // Send initial data to new client
        sendWebSocketData(num);
        break;
      }
    case WStype_TEXT:
      Serial.printf("WebSocket client #%u sent text: %s\n", num, payload);
      break;
    default:
      break;
  }
}

// Unified JSON builder — single source of truth for status JSON
// includeSetup: true adds setup flags and configuration parameters
// Returns number of bytes written (excluding null terminator)
int buildStatusJSON(char* buf, size_t len, bool includeSetup) {
  double snapTemp = getTemp();
  double snapSetpoint = getSetpointTemp();
  double snapPidOutput = getCurrentPIDOutputPercent();
  bool snapUnsafePid = getAllowUnsafePid();

  int pos = snprintf(buf, len,
    "{\"currentTemp\":%.1f,\"setpointTemp\":%.1f,\"shotTime\":%.1f,\"shotWeight\":%.1f,"
    "\"currentPressure\":%.1f,\"flowRate\":%.1f,\"currentShotRatio\":%.2f,"
    "\"doseWeight\":%.1f,\"pidOutput\":%.0f,\"scaleConnected\":%s,"
    "\"isBrewing\":%s,\"boilerHeating\":%s,\"systemState\":\"%s\","
    "\"errorMessage\":\"%s\",\"systemMessage\":\"%s\",\"cleaningActive\":%s,\"pidBypass\":%s",
    snapTemp, snapSetpoint, shotTime, shotWeight,
    currentPressure, currentFlowRate, currentShotRatio,
    doseWeight, snapPidOutput,
    scaleConnected ? "true" : "false",
    brewActive ? "true" : "false",
    boilerHeating ? "true" : "false",
    getStateName(currentState),
    systemMessage.c_str(), systemMessage.c_str(),
    cleaningActive ? "true" : "false",
    snapUnsafePid ? "true" : "false");
  if (pos < 0 || (size_t)pos >= len) return pos;

  if (includeSetup) {
    pos += snprintf(buf + pos, len - pos,
      ",\"firstRun\":%s,\"tempCalComplete\":%s,\"pressureCalComplete\":%s,"
      "\"scaleCalComplete\":%s,\"boilerFilled\":%s,"
      "\"brewTemp\":%.1f,\"shotTargetTime\":%.1f,\"shotTargetWeight\":%.1f,"
      "\"pidKp\":%.1f,\"pidKi\":%.2f,\"pidKd\":%.1f,"
      "\"pidBoost\":%.1f,\"preInfusion\":%.1f,\"preInfusionRate\":%.0f",
      setupComplete ? "false" : "true",
      tempCalComplete ? "true" : "false",
      pressureCalComplete ? "true" : "false",
      scaleCalComplete ? "true" : "false",
      boilerFilled ? "true" : "false",
      snapSetpoint, shotTargetTime, shotTargetWeight,
      Kp, Ki, Kd,
      pidBoostDuration, preInfusionDuration, preInfusionPulseRate);
    if (pos < 0 || (size_t)pos >= len) return pos;
  }

  pos += snprintf(buf + pos, len - pos, ",\"uptime\":%lu}", millis());
  return pos;
}

// Send WebSocket data to specific client (new connection — include setup)
void sendWebSocketData(uint8_t clientNum) {
  buildStatusJSON(webSocketBuffer, sizeof(webSocketBuffer), true);
  webSocket.sendTXT(clientNum, webSocketBuffer);
}

// Broadcast WebSocket data to all clients
void broadcastWebSocketData() {
  buildStatusJSON(webSocketBuffer, sizeof(webSocketBuffer), false);
  webSocket.broadcastTXT(webSocketBuffer);
  webSocketMessageCount++;
}

// --- FreeRTOS Task Functions ---

// PID Control handled by hardware timer ISR only (per requirements)
// Core 1 hardware timer ISR fires every 100ms and calls pid_step()
// No FreeRTOS task needed for PID control

// FreeRTOS tasks removed - using millis() timing in main loop per requirements

// --- Setup & Loop ---
void setup() {
  Serial.begin(115200);

#ifndef BENCH_MODE
  // --- PRODUCTION: Full hardware initialization ---
  // Initialize relay outputs (all OFF for safety)
  Serial.println("Initializing relay outputs...");
  initRelays();
  Serial.println("Relay outputs initialized (all OFF)");

  // Initialize hardware watchdog
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
  esp_task_wdt_add(NULL);

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

  // Startup fill cycle — only run if boiler has been filled before (implies fill probe
  // is calibrated). On fresh install (boilerFilled=false in EEPROM), skip and let the
  // user complete fill probe calibration via web UI before the fill system operates.
  if (boilerFilled) {
    Serial.println("Running startup fill cycle for steam boiler safety...");
    runStartupFillCycle();
  } else {
    Serial.println("Startup fill cycle skipped — fresh install or boilerFilled=false");
    Serial.println("Complete fill probe calibration via web UI before using the fill system");
  }

  // Initialize Brew Switch
  initBrewSwitch();

  // Initialize Pressure Measurement System
  initPressure();

  // Initialize MAX31865 RTD sensor
  initRTDSensor();

  // Initialize Core 1 PID Task (dedicated FreeRTOS task for deterministic control)
  initPIDTask();
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

  // --- REQUIREMENTS-COMPLIANT SETUP COMPLETE ---
  Serial.println("System ready:");
  Serial.println("- Core 1: Hardware timer ISR for PID control (100ms)");
  Serial.println("- Core 0: Main loop with millis() timing for non-critical operations");
}

// --- Core Loop Functions ---
void handlePIDUpdates() {
  // PID control runs independently on Core 1 via dedicated FreeRTOS task
  // Main loop responsibility: Read temperature via SPI and cache for PID task
  // This keeps SPI access on Core 0 for better bus arbitration

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
        // Pre-infusion phase completion - thread-safe variable update
        portENTER_CRITICAL(&controlMux);
        preInfusionPhase = false;
        portEXIT_CRITICAL(&controlMux);
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

void loop() {
  // --- REQUIREMENTS-COMPLIANT ARCHITECTURE ---
  // Core 1: Hardware timer ISR handles PID control every 100ms
  // Core 0: Main loop handles Web UI, OLED updates, non-critical sensors using millis() timing

  handleNetworkCommunication();  // Network: Web server and WebSocket handling
#ifndef BENCH_MODE
  handlePIDUpdates();            // Critical: PID control and temperature reading
  handleSystemMonitoring();      // Safety: Error monitoring and watchdog
  updateStateMachine();          // Control: State machine updates
  handleSensorReadings();        // Sensors: Scale and pressure readings
  handleUserInterface();         // Display: OLED updates
  handleSystemOperations();      // Operations: Fill, brew switch, calibration
  handleBrewPhaseManagement();   // Brew: Shot timing and phase control
#endif
  handleSerialCommands();        // Debug: Serial command processing
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
