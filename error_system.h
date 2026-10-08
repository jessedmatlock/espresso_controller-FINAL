// error_system.h — atomic error-flag bus. Safe to read/write from either
// core by construction (std::atomic<uint8_t>), unchanged from the original
// design. Extracted verbatim; no logic changes.
#pragma once
#include <Arduino.h>
#include <atomic>

#define ERR_FLAG_TEMP (1 << 0)          // Over-temperature error
#define ERR_FLAG_RTD (1 << 1)           // RTD sensor fault
#define ERR_FLAG_PID (1 << 2)           // PID control failure
#define ERR_FLAG_SCALE (1 << 3)         // Scale disconnected
#define ERR_FLAG_PRESSURE (1 << 4)      // Over-pressure error
#define ERR_FLAG_DISPLAY (1 << 5)       // Display initialization failed
#define ERR_FLAG_I2C (1 << 6)           // I2C communication failed
#define ERR_FLAG_LOOP_TIMEOUT (1 << 7)  // Main loop timeout

extern std::atomic<uint8_t> errorFlags;

// System message for display (OLED bottom line / web UI banner). Single
// task only. updateSystemMessage() derives the error-related text; other
// files (state_machine.cpp, web_api.cpp) assign non-error status text
// directly, exactly as in the original design.
extern String systemMessage;

inline uint8_t getErrorFlags() {
  return errorFlags.load(std::memory_order_acquire);
}
inline void setErrorFlag(uint8_t flag) {
  errorFlags.fetch_or(flag, std::memory_order_release);
}
inline void clearErrorFlag(uint8_t flag) {
  errorFlags.fetch_and(~flag, std::memory_order_release);
}
inline bool hasErrorFlag(uint8_t flag) {
  return (getErrorFlags() & flag) != 0;
}
inline bool hasCriticalErrorFlag() {
  return (getErrorFlags() & (ERR_FLAG_TEMP | ERR_FLAG_RTD | ERR_FLAG_PID)) != 0;
}

bool hasError();
bool isCriticalError();
bool isRecoverableError();
bool isDisplayOnlyError();

// Defined in state_machine.cpp (depends on setupComplete / relay state).
bool requiresManualReboot();
void emergencyStop();

// Consolidated error set/clear — does Serial logging and, for critical
// flags, calls emergencyStop(). pid_control.cpp's hot path (pid_step(),
// called every 100ms) deliberately uses setErrorFlag()/clearErrorFlag()
// directly instead, to avoid that Serial/emergencyStop overhead on every
// cycle — not because of a core boundary (there isn't one), but to keep
// the PID control path fast and defer logging to processPIDTaskLogs().
void setError(uint8_t flag);
void clearError(uint8_t flag);
void updateSystemMessage();
