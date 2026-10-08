// display.cpp — OLED partial-update rendering. Extracted verbatim; no logic
// changes. Single task only.
#include <Arduino.h>
#include <U8g2lib.h>
#include "error_system.h"
#include "shared_state.h"
#include "display.h"

// Hardware object — defined in the main .ino.
extern U8G2_SH1107_128X128_F_HW_I2C u8g2;

// --- Shared state this display reads ---
extern bool displayFailed;
extern bool setupComplete;
extern double shotTime;
extern double shotWeight;
extern bool scaleConnected;
extern bool brewActive;
extern bool fillActive;
extern double currentShotRatio;
extern double currentPressure;
extern double currentFlowRate;

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
  bool boilerHeatingNow = getBoilerHeatingState();
  if (boilerHeatingNow != lastDisplayedBoilerHeating) {
    u8g2.setDrawColor(0);  // Clear area
    u8g2.drawBox(90, 38, 35, 10);
    u8g2.setDrawColor(1);  // Draw text
    if (boilerHeatingNow) {
      u8g2.drawStr(90, 45, "HEAT");
    }
    lastDisplayedBoilerHeating = boilerHeatingNow;
    needsUpdate = true;
  }

  // Only update status icons if changed
  static bool lastDisplayedPidBoostActive = false;
  bool preInfusionActiveNow = getPreInfusionActive();
  bool pidBoostActiveNow = getPidBoostActive();
  if (brewActive != lastDisplayedBrewActive || fillActive != lastDisplayedFillActive ||
      preInfusionActiveNow != lastDisplayedPreInfusionActive || pidBoostActiveNow != lastDisplayedPidBoostActive) {
    u8g2.setDrawColor(0);  // Clear status line
    u8g2.drawBox(5, 53, 120, 10);
    u8g2.setDrawColor(1);  // Draw icons
    if (brewActive) {
      u8g2.drawStr(5, 60, "BREW");
    }
    if (fillActive) {
      u8g2.drawStr(35, 60, "FILL");
    }
    if (preInfusionActiveNow) {
      u8g2.drawStr(60, 60, "PRE");
    }
    if (pidBoostActiveNow) {
      u8g2.drawStr(90, 60, "BOOST");
    }
    lastDisplayedBrewActive = brewActive;
    lastDisplayedFillActive = fillActive;
    lastDisplayedPreInfusionActive = preInfusionActiveNow;
    lastDisplayedPidBoostActive = pidBoostActiveNow;
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
