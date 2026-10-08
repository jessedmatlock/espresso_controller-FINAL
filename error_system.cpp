// error_system.cpp — unchanged logic from the original setError()/clearError()
// consolidated error system (Rev 55 "R5"). Extracted verbatim.
#include "error_system.h"

std::atomic<uint8_t> errorFlags(0);
String systemMessage = "";

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
