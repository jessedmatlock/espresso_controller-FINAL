# Espresso Controller Session Notes - February 7, 2026

## Firmware Status: REVISION 55 - PRODUCTION READY

### Files Location
```
/Users/jessematlock/Downloads/0-ESPRESSO/espresso_controller-FINAL/
```

### Files in Project
- `espresso_controller-FINAL.ino` - Main firmware
- `web_ui_html.h` - HTML structure
- `web_ui_css.h` - Custom CSS
- `web_ui_js.h` - JavaScript functionality
- `bootstrap_custom.h` - Bootstrap CSS (~60KB)
- `bootstrap_js.h` - Bootstrap JS (~50KB)
- `apexcharts_js.h` - ApexCharts library (~581KB)

---

## Changes Made This Session

### Rev 52: Thread Safety
- Fixed `errorMessage` race condition with atomic error flags
- Added `ERR_FLAG_TEMP`, `ERR_FLAG_RTD`, `ERR_FLAG_PID`, `ERR_FLAG_SCALE`, `ERR_FLAG_PRESSURE`
- Added `hasCriticalErrorFlag()` and `hasErrorFlag()` helpers
- Updated `pid_step()` to use atomic flags

### Rev 53: Simplified First-Run
- Temperature probe now auto-confirms on successful initialization
- No manual wizard step required
- Validates no RTD faults and reading in 32-250°F range

### Rev 54: Dual Boiler Architecture
- Separated steam boiler (pressostat) from brew boiler (PID)
- Removed `boilerFilled` from PID requirements
- `setupComplete = (tempCalComplete && pressureCalComplete)` only
- Added `runStartupFillCycle()` for steam boiler safety on boot
- Startup fill has 2-minute timeout with watchdog feeding

### Rev 55: Safety & Performance Review
**Safety Fixes (S-1 through S-5):**
- S-1: Removed relay toggle test in setup() - prevents momentary actuation
- S-2: Enforced 250ms minimum pre-infusion pulse rate - protects pump/relay
- S-3: Added critical section to emergencyStop() - atomic state changes
- S-4: Added brew/fill solenoid interlocks - prevents simultaneous activation
- S-5: Unified temperature rate limit to MAX_TEMP_RATE=5°F/s constant

**Performance Fixes (P-1 through P-5):**
- P-1: Changed PID to derivative-on-measurement - prevents setpoint change kick
- P-2: Documented PID_OUTPUT_SCALE constant with gain calculations
- P-3: Increased WebSocket buffer from 512 to 768 bytes - prevents truncation
- P-4: Moved scale stability variables to global scope - improved testability
- P-5: Reset pressure calibration accumulators at start - prevents stale values

### Compilation Fixes
- Added forward declarations for all functions
- Changed `setSampleRate()` to `setRate()` for NAU7802 library

---

## Required Libraries (Arduino Library Manager)

1. **WebSockets** by Markus Sattler
2. **U8g2** by oliver
3. **Adafruit NAU7802**
4. **Adafruit MAX31865** (should already be installed)
5. **Adafruit BusIO** (dependency)

---

## Arduino IDE Settings for Arduino Nano ESP32

| Setting | Value |
|---------|-------|
| Board | Arduino Nano ESP32 |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz (WiFi) |
| Core Debug Level | None |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB |
| **Partition Scheme** | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| Upload Speed | 921600 |
| USB Mode | Hardware CDC and JTAG |

**CRITICAL:** Must use "Huge APP" partition scheme - firmware is ~1.5MB

---

## Upload Process

1. Remove ESP32 from GPIO expansion board
2. Hold BOOT button, plug in USB, release BOOT
3. Select port in Tools → Port
4. Click Upload
5. After successful upload, plug back into expansion board

---

## Power Requirements

USB power is **NOT sufficient** for full system. Need external 5V 2A+ supply to expansion board when:
- Relays connected (~400mA for 4 relays)
- NAU7802 + load cell
- OLED display
- MAX31865
- Pressure transducer

---

## Hardware Pin Mapping

| Pin | Function |
|-----|----------|
| GPIO21 | SPI CS (MAX31865 temp sensor) |
| GPIO2 | Fill Probe (analog input) |
| GPIO3 | Brew Switch (momentary, NO to GND) |
| GPIO8 | Relay CH1 - Brew Solenoid |
| GPIO9 | Relay CH2 - Pump |
| GPIO10 | Relay CH3 - Fill Solenoid |
| GPIO17 | Relay CH4 - Boiler SSR |
| GPIO11 | I2C SDA (OLED, NAU7802) |
| GPIO12 | I2C SCL (OLED, NAU7802) |
| GPIO1 | Pressure Transducer (analog) |

---

## First-Run Setup Requirements

| Requirement | How Completed | Blocks PID? |
|-------------|---------------|-------------|
| Temperature Probe | Auto on boot | Yes |
| Pressure Transducer | Web UI calibration wizard | Yes |
| Steam Boiler Fill | Runs automatically at startup | No |

---

## API Endpoints (19 HTTP + 1 WebSocket)

- `/` - Web UI
- `/status` - JSON status
- `/set` - Set brew parameters
- `/setDose` - Set dose weight
- `/setPID` - Set PID parameters
- `/tare` - Tare scale
- `/calibrate/status` - Calibration state
- `/calibrate/fill/start` - Start fill probe cal
- `/calibrate/scale/start` - Start scale cal
- `/calibrate/scale/complete` - Complete scale cal
- `/calibrate/next` - Advance cal step
- `/calibrate/pressure/start` - Start pressure cal
- `/calibrate/pressure/next` - Advance pressure cal
- `/calibrate/pressure/setValue` - Set known pressure
- `/calibrate/pump/start` - Manual pump ON
- `/calibrate/pump/stop` - Manual pump OFF
- `/calibrate/temp/complete` - Manual temp confirm (usually not needed)
- `/startCleaning` - Start cleaning cycle
- `/stopCleaning` - Stop cleaning cycle
- `ws://:81/` - WebSocket real-time data

---

## Safety Features

- Over-temperature cutoff: 225°F with 10°F hysteresis auto-recovery
- Over-pressure cutoff: 12 BAR, pump locked until <11 BAR
- Max brew timeout: 60 seconds
- Max fill timeout: 60 seconds (startup: 2 minutes)
- Hardware watchdog in main loop
- Emergency stop on critical errors
- PID disabled until calibrations complete
- Startup steam boiler fill for pressostat safety

---

## Architecture

- **Core 0**: Main loop (Web UI, OLED, sensors, state machine)
- **Core 1**: PID task (100ms deterministic timing, highest priority)
- Temperature read on Core 0, cached for Core 1 (avoids SPI bus conflicts)
- All timing uses non-blocking `millis()` based approach
- Mutex protection for shared variables

---

## EEPROM Layout (66 bytes, addresses 0-65)

| Address | Parameter |
|---------|-----------|
| 0-1 | Signature (0xABCD) |
| 2-5 | Brew Temperature |
| 6-9 | Shot Target Time |
| 10-13 | Shot Target Weight |
| 14-17 | Pre-infusion Duration |
| 18-21 | Scale Calibration |
| 22-25 | PID Boost Duration |
| 26-29 | PID Kp |
| 30-33 | PID Ki |
| 34-37 | PID Kd |
| 38-41 | Fill Probe Threshold |
| 42-45 | Pre-infusion Pulse Rate |
| 46-49 | Pressure Offset |
| 50-53 | Pressure Scale |
| 54-57 | Dose Weight |
| 58-61 | Scale Zero Offset |
| 62 | Setup Complete |
| 63 | Temp Cal Complete |
| 64 | Pressure Cal Complete |
| 65 | Boiler Filled |

---

## Next Steps

1. Provide 5V 2A+ external power to expansion board
2. Test full system boot with all peripherals
3. Run pressure calibration wizard
4. Test brewing functionality
