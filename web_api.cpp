// web_api.cpp — HTTP handlers, WebSocket, and buildStatusJSON(). Extracted
// verbatim with one addition: handleRoot() now sends a Cache-Control header
// on the static page assets (D1 from the architecture review) — the content
// itself is unchanged PROGMEM data, so this only affects re-fetch behavior,
// not what's served. Single task only.
//
// The six web-asset headers (~700KB of PROGMEM HTML/CSS/JS) are included
// ONLY here, not in the main .ino — each is a `const char[]` with internal
// linkage, so including them in two translation units would silently
// duplicate that data in flash. handleRoot() is the only function that
// references them, so this is their one and only include site.
#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "bootstrap_custom.h"
#include "bootstrap_js.h"
#include "apexcharts_js.h"
#include "web_ui_css.h"
#include "web_ui_html.h"
#include "web_ui_js.h"
#include "config.h"
#include "error_system.h"
#include "shared_state.h"
#include "state_machine.h"
#include "calibration.h"
#include "relays.h"
#include "sensors.h"
#include "web_api.h"

// --- Hardware/network objects — defined in the main .ino ---
extern WebServer server;
extern WebSocketsServer webSocket;

// --- Helpers defined in the main .ino ---
void recomputeSetupComplete();
void startCleaningCycle();
void stopCleaningCycle();

// --- Shot/brew parameters (shared with eeprom_store.cpp/state_machine.cpp) ---
extern double originalSetpointTemp;
extern double shotTargetTime;
extern double shotTargetWeight;
extern double preInfusionDuration;
extern double pidBoostDuration;
extern double preInfusionPulseRate;
extern double doseWeight;
extern bool eepromDirty;

// --- Pressure calibration value (user-entered reference during cal) ---
extern double pressureHighValue;

// --- Setup-flow flags ---
extern bool tempCalComplete;
extern bool pressureCalComplete;
extern bool scaleCalComplete;
extern bool setupComplete;
extern bool boilerFilled;

// --- Status/telemetry fields read into the JSON payload ---
extern double shotTime;
extern double shotWeight;
extern bool scaleConnected;
extern bool brewActive;
extern double currentPressure;
extern double currentFlowRate;
extern double currentShotRatio;
extern bool cleaningActive;
extern SystemState currentState;

// --- Performance counters (not static — see their declaration in the main .ino) ---
extern unsigned long webRequestCount;
extern unsigned long totalWebResponseTime;
extern unsigned long maxWebResponseTime;
extern unsigned long webSocketMessageCount;

// Pre-allocated JSON buffers — avoids per-request heap churn.
static char jsonStatusBuffer[768];
static char webSocketBuffer[768];

void handleRoot() {
  // Stream HTML components directly to avoid large heap allocation
  // This eliminates the need for 48KB memory reservation

  // Static PROGMEM content only changes between firmware flashes — cache it
  // client-side for a day instead of re-sending ~700KB on every page load.
  server.sendHeader("Cache-Control", "public, max-age=86400");

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
    // This is now the only way to clear a critical RTD fault — a human has
    // just confirmed the probe gives a sane reading, rather than it silently
    // self-clearing the instant one good reading comes back (see sensors.cpp).
    clearError(ERR_FLAG_RTD);

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

// Kp/Ki/Kd now live behind mutex-protected accessors in shared_state.cpp
// (not bare globals), so ParamDef takes a setter function pointer instead of
// a raw double* — this is what actually closes off the ability to write
// these three gains without going through the lock.
static void setPidBoostDurationParam(double v) { pidBoostDuration = v; }
static void setPreInfusionDurationParam(double v) { preInfusionDuration = v; }
static void setPreInfusionPulseRateParam(double v) { preInfusionPulseRate = v; }

struct ParamDef {
  const char* argName;
  void (*setter)(double);
  double min;
  double max;          // 0 means use shotTargetTime as dynamic max
  const char* label;
  const char* fmt;     // printf format for response (e.g. "%.1f", "%.2f")
};

static const ParamDef pidParams[] = {
  {"kp",          setKp,                       0.1,   200.0, "Kp",           "%.1f"},
  {"ki",          setKi,                       0.0,   10.0,  "Ki",           "%.2f"},
  {"kd",          setKd,                       0.0,   500.0, "Kd",           "%.1f"},
  {"boost",       setPidBoostDurationParam,    0.0,   0,     "Boost",        "%.1fs"},
  {"preinfusion", setPreInfusionDurationParam, 0.0,   0,     "Pre-infusion", "%.1fs"},
  {"rate",        setPreInfusionPulseRateParam, 100.0, 1000.0, "Rate",       "%.0fms"},
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

    p.setter(val);
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
  bool snapBoilerHeating = getBoilerHeatingState();

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
    snapBoilerHeating ? "true" : "false",
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
      getKp(), getKi(), getKd(),
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
