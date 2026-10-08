// web_api.h — HTTP handlers, WebSocket, and the unified JSON status builder.
// Single task only.
#pragma once
#include <Arduino.h>
#include <WebSocketsServer.h>

void handleRoot();
void handleStatus();
void handleTare();
void handleCalibrateStatus();
void handleCalibrateFillStart();
void handleCalibrateScaleStart();
void handleCalibrateNext();
void handleCalibrateScaleComplete();
void handleCalibratePressureStart();
void handleCalibratePressureNext();
void handleCalibratePumpStart();
void handleCalibratePumpStop();
void handleCalibrateTempComplete();
void handleSetPressureCalibrationValue();
void handleCalibrateCancel();
void handleSetpoint();
void handleSetDoseWeight();
void handleSetParams();
void handleSetPIDParameters();
void handleSetSafety();
void handleStartCleaning();
void handleStopCleaning();

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
void sendWebSocketData(uint8_t clientNum);
void broadcastWebSocketData();

// buildStatusJSON: single source of truth for both /status and WebSocket
// payloads. includeSetup adds setup/calibration flags and configuration
// parameters (sent on first connect and HTTP polls, omitted from the 100ms
// broadcast to keep it small). Returns bytes written (excluding the null
// terminator).
int buildStatusJSON(char* buf, size_t len, bool includeSetup);
