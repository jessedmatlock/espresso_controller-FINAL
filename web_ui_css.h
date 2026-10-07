/*
 * web_ui_css.h
 * Custom CSS styles for ESP32 Espresso Controller Web UI
 * 
 * This file contains the custom styling from new-graph.html
 * Used in conjunction with Bootstrap CSS (bootstrap_custom.h)
 */

#ifndef WEB_UI_CSS_H
#define WEB_UI_CSS_H

const char web_ui_css[] PROGMEM = R"rawliteral(
/* Chart container */
#chart {
  max-width: 100%;
  width: 100%;
  margin: 0px auto;
}

/* Base body styling */
body {
  font-family: "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background: #293c47;
  color: #f1f5f9;
  line-height: 1.6;
  padding: 20px;
}

/* Dark theme card backgrounds */
.bg-dark-1 {
  background: #293c47 !important;
}

.bg-dark-2 {
  background: #21313a !important;
}

.card .card.bg-dark {
  background: rgba(0, 0, 0, 0.2) !important;
  color: #f1f5f9 !important;
}

/* Colored indicator lines under metrics */
.line {
  position: relative;
  height: 5px;
  width: 100px;
  margin: 5px auto;
  border-radius: 5px;
  background: #84919a;
}

.line.dose {
  background: #7c6357;
}

.line.time {
  background: #555;
}

.line.rate {
  background: #888;
}

.line.ratio {
  background: #293c47;
}

.line.weight {
  background: #4ee29b;
}

.line.pressure {
  background: #00aef5;
}

.line.temp {
  background: #ffcb4e;
}

.line.pid {
  background: #993b3b;
}

.line.status {
  background: #777;
}

/* Calibration status styling */
.calibration-status {
  margin-top: 10px;
  padding: 10px;
  background: rgba(0, 0, 0, 0.2);
  border-radius: 5px;
  font-size: 0.9em;
}

/* Message styling */
#message, #advancedMessage {
  margin-top: 10px;
  padding: 8px;
  border-radius: 4px;
  text-align: center;
}

#message.success, #advancedMessage.success {
  background: rgba(78, 226, 155, 0.2);
  color: #4ee29b;
}

#message.error, #advancedMessage.error {
  background: rgba(220, 53, 69, 0.2);
  color: #dc3545;
}

/* First run setup banner */
.first-run-banner {
  background: rgba(0, 0, 0, 0.2) !important;
  color: white;
  padding: 20px;
  border-radius: 10px;
  margin-bottom: 20px;
  text-align: center;
}

.first-run-banner h2 {
  margin-bottom: 10px;
}

.first-run-banner .required-badge {
  background: #dc3545;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 0.8em;
  margin-left: 5px;
}

.first-run-banner .optional-badge {
  background: #6c757d;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 0.8em;
  margin-left: 5px;
}
)rawliteral";

#endif // WEB_UI_CSS_H
