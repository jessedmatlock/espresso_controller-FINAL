Arduino ESP32 Nano based Espresso Controller for customized Dalla Corte Mini espresso machine
Brew by weight or time - drip tray has built in scale
Custom PID programming to mitigate inrush of cooler water during initial brew
Adjustable Pre-Infusion
WIFI connection to tablet for full graphing of shot params
and more...

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
