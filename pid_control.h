// pid_control.h — PID temperature control math. Runs as a plain function
// called from the single main task's loop (gated internally to 100ms, same
// self-gating pattern as every other periodic call in this codebase) — not
// a separate FreeRTOS task. There is only one execution context in this
// firmware, so nothing here needs mutex protection; see shared_state.h for
// why its accessors no longer lock either.
#pragma once

// Call unconditionally every main-loop iteration; internally gated to
// CONTROL_INTERVAL_MS (100ms). Runs the PID cycle, or — while setup is
// incomplete — holds the heater off and keeps the integral term at zero.
void updatePidControl();

// Resets the PID integral term (private storage in pid_control.cpp).
// Called from state_machine's startBrewRoutine() at the start of each brew.
void resetPidIntegral();
