/*
 * web_ui_html.h
 * HTML structure for ESP32 Espresso Controller Web UI
 * 
 * This file contains the HTML layout from new-graph.html
 * Styled by Bootstrap (bootstrap_custom.h) and custom CSS (web_ui_css.h)
 * Functionality provided by web_ui_js.h
 */

#ifndef WEB_UI_HTML_H
#define WEB_UI_HTML_H

const char web_ui_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Espresso Controller</title>
    <style>
)rawliteral";

// Note: Bootstrap CSS and custom CSS will be injected here by handleRoot()

const char web_ui_html_body[] PROGMEM = R"rawliteral(
    </style>
</head>
<body>

<!-- First Run Setup Banner (hidden by default, shown via JS when firstRun=true) -->
<div id="firstRunBanner" class="first-run-banner d-none">
    <h2>First Run Setup Required</h2>
    <p>Complete the required calibrations below to enable PID heating control.</p>
    <div class="mb-2">
        <span id="tempCalStatus">❌ Temperature Probe</span> <span class="required-badge">REQUIRED</span>
    </div>
    <div class="mb-2">
        <span id="pressureCalStatus">❌ Pressure Transducer</span> <span class="required-badge">REQUIRED</span>
    </div>
    <div class="mb-2">
        <span id="scaleCalStatus">Scale</span> <span class="optional-badge">OPTIONAL</span>
    </div>
    <div class="mb-2">
        <span id="fillCalStatus">Fill Probe</span> <span class="optional-badge">OPTIONAL</span>
    </div>
</div>

<div class="card bg-dark-1">
    <!-- Error/System Message Banner -->
    <div id="errorBanner" class="alert alert-warning alert-dismissible d-none" role="alert">
        <span class="error-icon">⚠️</span> <span id="errorText"></span>
    </div>

    <!-- Top Row: Shot Metrics -->
    <div class="row text-center g-3 p-3">
        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="doseWeightDisplay">--.- <span class="fw-light fs-6 fst-italic">g</span></div>
                <div class="line dose"></div>
                <div style="color:#84919A">Dose</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="shotRatio">--:1 <span class="fw-light fs-6 fst-italic"></span></div>
                <div class="line ratio"></div>
                <div style="color:#84919A">Shot Ratio</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="flowRate">--.- <span class="fw-light fs-6 fst-italic">ml/s</span></div>
                <div class="line rate"></div>
                <div style="color:#84919A">Flow Rate</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="shotWeight">--.- <span class="fw-light fs-6 fst-italic">g</span></div>
                <div class="line weight"></div>
                <div style="color:#84919A">Shot Weight</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="shotTime">--.- <span class="fw-light fs-6 fst-italic">s</span></div>
                <div class="line time"></div>
                <div style="color:#84919A">Shot Time</div>
            </div>
        </div>
    </div>

    <!-- Second Row: System Metrics + Controls -->
    <div class="row text-center g-3 px-3">
        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="currentPressure">--.- <span class="fw-light fs-6 fst-italic">BAR</span></div>
                <div class="line pressure"></div>
                <div style="color:#84919A">Pressure</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="pidOutput">-- <span class="fw-light fs-6 fst-italic">%</span></div>
                <div class="line pid"></div>
                <div style="color:#84919A">PID Output</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-1" id="currentTemp">--.- <span class="fw-light fs-6 fst-italic">°F</span></div>
                <div class="line temp"></div>
                <div style="color:#84919A">Temp.</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <div class="fs-3 p-2" id="systemState"><span class="fw-light fs-6 fst-italic">--</span></div>
                <div class="line status"></div>
                <div style="color:#84919A">Status</div>
            </div>
        </div>

        <div class="col">
            <div class="card bg-dark p-2">
                <button class="btn btn-secondary d-block w-100 mb-4" onclick="tareScale()">Tare Scale</button>
                <button class="btn btn-success w-100" onclick="startCleaning()" id="startCleaningBtn">Start Cleaning Cycle</button>
                <button class="btn btn-warning w-100" onclick="stopCleaning()" id="stopCleaningBtn" style="display: none;">Stop Cleaning</button>
            </div>
        </div>
    </div>


</div>

<div class="row d-block">
    <div id="chart" class="d-block"></div>
    <button class="btn btn-secondary btn-sm d-block" style="width:200px;margin: 0 auto;" type="button" data-bs-toggle="collapse" data-bs-target="#collapseSettings" aria-expanded="false" aria-controls="collapseSettings">
        ⚙ Settings
    </button>
</div>

<!-- Collapsible Settings Panel -->
<div class="collapse mt-3" id="collapseSettings">
    <div class="row">
        <!-- Shot Parameters Card -->
        <div class="col-lg-4">
            <div class="card bg-dark-2 text-light">
                <div class="card-header">Shot Parameters</div>
                <div class="card-body">
                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="brewTemp" class="col-sm-6 col-form-label">Brew Temp</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="brewTemp" placeholder="200.5" min="180" max="220" step="0.5">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="shotTargetTime" class="col-sm-6 col-form-label">Target Shot Time</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="shotTargetTime" placeholder="34" min="0" max="60" step="1">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="doseWeightInput" class="col-sm-6 col-form-label">Dose Weight</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="doseWeightInput" placeholder="16.5" min="10" max="30" step="0.5">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="shotTargetWeight" class="col-sm-6 col-form-label">Target Shot Weight</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="shotTargetWeight" placeholder="34" min="0" max="60" step="0.5">
                            </div>
                        </div>
                    </div>
                </div>
                <div class="card-footer">
                    <button onclick="updateParameters()" class="btn btn-success d-block w-100">Save Shot Params</button>
                    <div id="message"></div>
                </div>
            </div>
        </div>

        <!-- Advanced Settings Card -->
        <div class="col-lg-4">
            <div class="card bg-dark-2 text-light">
                <div class="card-header">Advanced Settings</div>
                <div class="card-body">
                    <h5 class="card-title">PID Settings</h5>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="pidKp" class="col-sm-6 col-form-label">PID Kp</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="pidKp" min="0.1" max="200" step="0.1" placeholder="40.0">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="pidKi" class="col-sm-6 col-form-label">PID Ki</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="pidKi" min="0.0" max="10" step="0.01" placeholder="0.8">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="pidKd" class="col-sm-6 col-form-label">PID Kd</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="pidKd" min="0.0" max="500" step="0.1" placeholder="120.0">
                            </div>
                        </div>
                    </div>

                    <hr />
                    <h5 class="card-title">PID Boost &amp; Pre-Infusion</h5>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="pidBoost" class="col-sm-6 col-form-label">PID Boost (seconds)</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="pidBoost" min="0" max="30" step="0.5" placeholder="4.0">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="preInfusion" class="col-sm-6 col-form-label">Pre-infusion (secs)</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="preInfusion" min="0" max="30" step="0.5" placeholder="8.0">
                            </div>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="preInfusionRate" class="col-sm-6 col-form-label">Pre-infusion Rate (ms)</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="preInfusionRate" min="100" max="2000" step="50" placeholder="500">
                            </div>
                        </div>
                    </div>
                </div>
                <div class="card-footer">
                    <button onclick="updateAdvancedParameters()" class="btn btn-success d-block w-100 mb-3">Save Advanced Settings</button>
                    <div id="advancedMessage" class="alert alert-danger d-none"></div>
                </div>
            </div>
        </div>

        <!-- Calibration Settings Card -->
        <div class="col-lg-4">
            <div class="card bg-dark-2 text-light">
                <div class="card-header">System Calibration Settings</div>
                <div class="card-body">
                    <h5 class="card-title">Temperature Probe</h5>
                    <button onclick="confirmTempProbe()" class="btn btn-secondary w-100 mb-3">Confirm Temp Probe</button>

                    <hr />
                    <h5 class="card-title">Fill Probe</h5>
                    <button onclick="startFillCalibration()" class="btn btn-secondary w-100 mb-3">Calibrate Fill Probe</button>
                    <button onclick="nextCalibrationStep()" class="btn btn-secondary w-100 mb-3">Next Step</button>
                    
                    <hr />
                    <h5 class="card-title">Scale</h5>
                    <button onclick="startScaleCalibration()" class="btn btn-secondary w-100 mb-3">Calibrate Scale</button>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="knownWeight" class="col-sm-6 col-form-label">Known Weight</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="knownWeight" min="1" max="500" step="0.1" placeholder="100.0">
                            </div>
                        </div>
                    </div>
                    <button onclick="completeScaleCalibration()" class="btn btn-success w-100">Complete Scale Cal</button>

                    <hr />
                    <h5 class="card-title">Pressure</h5>
                    <button onclick="startPressureCalibration()" class="btn btn-secondary w-100 mb-3">Calibrate Pressure</button>
                    <button onclick="nextPressureCalibrationStep()" class="btn btn-secondary w-100 mb-3">Next Pressure Step</button>

                    <div class="row">
                        <div class="col-6">
                            <button onclick="startCalibrationPump()" class="btn btn-primary w-100 mb-3">Start Pump</button>
                        </div>
                        <div class="col-6">
                            <button onclick="stopCalibrationPump()" class="btn btn-danger w-100 mb-3">Stop Pump</button>
                        </div>
                    </div>

                    <div class="mb-3">
                        <div class="row mb-3">
                            <label for="actualPressure" class="col-sm-6 col-form-label">Actual Pressure (BAR)</label>
                            <div class="col-sm-6">
                                <input type="number" class="form-control" id="actualPressure" min="1" max="15" step="0.1" placeholder="8.0">
                            </div>
                        </div>
                    </div>
                    <button onclick="setPressureCalibrationValue()" class="btn btn-success w-100">Set Pressure Value</button>
                    
                    <div id="calibrationStatus" class="calibration-status"><em>No active calibration</em></div>

                    <hr />
                    <button onclick="cancelCalibration()" class="btn btn-outline-danger w-100">Cancel Calibration</button>
                </div>
            </div>
        </div>
    </div>
</div>

<script>
)rawliteral";

// Note: JavaScript will be injected here by handleRoot()
// Then Bootstrap JS and ApexCharts JS will be added

const char web_ui_html_footer[] PROGMEM = R"rawliteral(
</script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_HTML_H
