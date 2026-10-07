// control_task.h — Core 1 PID control. Everything in control_task.cpp runs
// ONLY on the ControlTask (Core 1, explicitly pinned). It touches hardware
// only through set_boiler_element()/set_pump() (relay layer), reads inputs
// only through shared_state.h accessors, and never calls Serial, EEPROM, or
// any SPI/I2C function directly — those stay on Core 0 by construction.
#pragma once

// Creates the ControlTask pinned to Core 1. Called once from setup().
void initPIDTask();

// Resets the PID integral term (pid_integral is private to control_task.cpp).
// Called from Core 0 (state_machine's startBrewRoutine()) at the start of
// each brew, and internally by the task itself while setup is incomplete —
// same two call sites and same effect as the original inline
// portENTER_CRITICAL(&pidMux) block.
void resetPidIntegral();
