// state_machine.h — the SystemState enum and the stateMux-protected
// accessors, shared by both control_task.cpp (reads getState() to gate PID
// boost behavior) and the main sketch (owns the full state machine). Full
// transition logic stays in the main .ino for this pass; only the type and
// accessor declarations are centralized here so both sides compile against
// the same definition.
#pragma once

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

SystemState getState();
void changeState(SystemState newState);
const char* getStateName(SystemState state);
