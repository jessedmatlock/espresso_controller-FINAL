// config.h — Pin assignments, EEPROM memory map, and cross-cutting safety
// constants shared by more than one subsystem file. No logic lives here.
//
// Extracted verbatim from espresso_controller-FINAL.ino during the Core 0/Core 1
// concurrency refactor — values unchanged from the original single-file version.
#pragma once

// Uncomment for bench testing WITHOUT hardware connected:
// Skips ALL hardware init (relays, sensors, display, PID task, watchdog).
// Only WiFi AP, web server, WebSocket, and serial remain active.
// Use to confirm web UI and serial before installing MCU onto expansion board.
// Comment out when installing onto hardware for sensor commissioning or production.
// Defined here (not in the main .ino) so every .cpp file in the sketch that
// includes config.h sees the same setting — relays.cpp's set_boiler_element()
// in particular depends on this.
//#define BENCH_MODE

// --- Pin Configuration ---
const int PIN_TEMP_CS = 21;       // SPI CS for MAX31865 RTD (D10/GPIO21)
const int PIN_FILL_PROBE = 2;     // Fill level probe, analog (A1/GPIO2)
const int PIN_BREW_SWITCH = 3;    // Brew momentary switch, NO to GND (A2/GPIO3)
const int PIN_PRESSURE = 1;       // Pressure transducer, analog (A0/GPIO1)

// 4-Channel Relay Control (3.3V trigger, 12V coil, switches 10A @ 120VAC)
const int PIN_RELAY_CH1 = 8;   // Ch1: Brew Solenoid (D5/GPIO8)
const int PIN_RELAY_CH2 = 9;   // Ch2: Pump (D6/GPIO9)
const int PIN_RELAY_CH3 = 10;  // Ch3: Fill Solenoid (D7/GPIO10)
const int PIN_RELAY_CH4 = 17;  // Ch4: Boiler Element SSR (D8/GPIO17)

// RTD configuration constants
const float RREF = 430.0;      // Reference resistor (430 ohms for PT100)
const float RNOMINAL = 100.0;  // PT100 nominal resistance at 0°C

// Pressure transducer constants - direct 3.3V connection
const double ADC_MAX_VOLTAGE = 3.3;
const int ADC_RESOLUTION = 4096;  // 12-bit ADC

// --- Cross-cutting safety constants (read by both sensors.cpp and control_task.cpp) ---
const double safetyTemp = 225.0;   // °F, emergency cutoff
const double minTemp = 32.0;       // °F, minimum valid reading (0°C)
const double MAX_TEMP_RATE = 5.0;  // °F/s, unified rate-of-change safety limit
const double TEMP_FILTER_ALPHA = 0.1;  // EMA filter coefficient for PID input
const double integralMax = 1000.0;     // PID integral windup protection limit

const double PRESSURE_LOCKOUT_HIGH = 12.0;  // BAR, enter pump lockout
const double PRESSURE_LOCKOUT_LOW = 11.0;   // BAR, exit pump lockout (hysteresis)

const unsigned long CONTROL_INTERVAL_MS = 100;  // PID task cycle interval
const double maxBrewTime = 60.0;                // seconds, absolute brew safety timeout
const unsigned long FILL_MAX_DURATION = 60000;  // ms, normal (post-boot) fill timeout
const unsigned long STARTUP_FILL_TIMEOUT = 120000;  // ms, one-time never-filled-before boot timeout

const unsigned long LOOP_TIMEOUT = 5000;  // ms, hardware watchdog timeout
const unsigned long PID_JITTER_THRESHOLD = 105;  // ms, alert if PID cycle exceeds this

// --- EEPROM Storage Structure ---
const uint16_t EEPROM_SIGNATURE = 0xABCD;
const int EEPROM_SIZE = 512;

// EEPROM Memory Map — 72 bytes total, addresses 0-71 (unchanged from original)
const int ADDR_SIGNATURE = 0;               // 0-1: Signature 0xABCD
const int ADDR_BREW_TEMP = 2;               // 2-5: Brew Temp (°F)
const int ADDR_SHOT_TIME = 6;               // 6-9: Shot Time (s)
const int ADDR_SHOT_WEIGHT = 10;            // 10-13: Shot Weight (g)
const int ADDR_PREINFUSION = 14;            // 14-17: Pre-infusion Time (s)
const int ADDR_SCALE_CAL = 18;              // 18-21: Scale Calibration Factor
const int ADDR_PID_BOOST = 22;              // 22-25: PID Boost (s)
const int ADDR_PID_KP = 26;                 // 26-29: PID Kp gain
const int ADDR_PID_KI = 30;                 // 30-33: PID Ki gain
const int ADDR_PID_KD = 34;                 // 34-37: PID Kd gain
const int ADDR_FILL_THRESHOLD = 38;         // 38-41: Fill Probe Threshold
const int ADDR_PREINFUSION_RATE = 42;       // 42-45: Pre-infusion Pulse Rate (ms)
const int ADDR_PRESSURE_OFFSET = 46;        // 46-49: Pressure Calibration Offset
const int ADDR_PRESSURE_SCALE = 50;         // 50-53: Pressure Calibration Scale
const int ADDR_DOSE_WEIGHT = 54;            // 54-57: Dose Weight (g)
const int ADDR_SCALE_ZERO_OFFSET = 58;      // 58-61: Scale Zero Offset (NAU7802)
const int ADDR_SETUP_COMPLETE = 62;         // 62: Setup Complete flag
const int ADDR_TEMP_CAL_COMPLETE = 63;      // 63: Temperature Calibration Complete
const int ADDR_PRESSURE_CAL_COMPLETE = 64;  // 64: Pressure Calibration Complete
const int ADDR_BOILER_FILLED = 65;          // 65: Boiler Filled at least once
const int ADDR_ALLOW_UNSAFE_PID = 66;       // 66: Allow PID bypass during setup
const int ADDR_SCALE_CAL_COMPLETE = 67;     // 67: Scale Calibration Complete
const int ADDR_CLEANING_CYCLES = 68;        // 68-71: Cleaning Cycle Count (int)
