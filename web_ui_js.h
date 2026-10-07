/*
 * web_ui_js.h
 * JavaScript functionality for ESP32 Espresso Controller Web UI
 * 
 * This file contains all client-side logic:
 * - WebSocket connection for real-time data
 * - ApexCharts initialization and updates
 * - API calls for parameter updates and calibration
 * - First-run setup detection and UI updates
 */

#ifndef WEB_UI_JS_H
#define WEB_UI_JS_H

const char web_ui_js[] PROGMEM = R"rawliteral(
    // Chart variables
    var chart;
    var chartInitialized = false;
    var tempData = [];
    var pressureData = [];
    var flowData = [];
    var weightData = [];
    var pidData = [];
    var startTime = null;
    var ws = null;
    var wsReconnectAttempts = 0;
    var maxReconnectAttempts = 10;

    // Initialize ApexCharts
    function initChart() {
        var isMobile = window.innerWidth < 768;

        var options = {
            series: [
                { name: 'Temperature (°F)', type: 'area', data: tempData },
                { name: 'Pressure (BAR)', type: 'area', data: pressureData },
                { name: 'Flow Rate (ml/s)', type: 'area', data: flowData },
                { name: 'Weight (g)', type: 'area', data: weightData },
                { name: 'PID Output (%)', type: 'area', data: pidData }
            ],
            chart: {
                height: isMobile ? 300 : 500,
                width: '100%',
                type: 'area',
                stacked: false,
                animations: { enabled: false },
                toolbar: { show: false },
                zoom: { enabled: false },
                background: '#293C47'
            },
            colors: ['#FFCB4E', '#00AEF5', '#888888', '#4EE29B', '#993b3b'],
            stroke: {
                curve: 'monotoneCubic',
                width: [5, 2, 2, 5, 1]
            },
            fill: {
                colors: undefined,
                opacity: 0.15,
                type: 'solid',
                gradient: {
                    shade: 'dark',
                    type: 'horizontal',
                    shadeIntensity: 0.15,
                    gradientToColors: undefined,
                    inverseColors: true,
                    opacityFrom: 1,
                    opacityTo: 1,
                    stops: [0, 50, 100],
                    colorStops: []
                }
            },
            markers: {
                size: 5,
                colors: undefined,
                strokeColors: '#293C47',
                strokeWidth: 3,
                strokeOpacity: 1,
                strokeDashArray: 0,
                fillOpacity: 1,
                discrete: [],
                shape: 'circle',
                offsetX: 0,
                offsetY: 0,
                onClick: undefined,
                onDblClick: undefined,
                showNullDataPoints: true,
                hover: {
                    size: undefined,
                    sizeOffset: 3
                }
            },
            dataLabels: { enabled: false },
            xaxis: {
                type: 'numeric',
                title: { text: 'Time (seconds)' },
                tickAmount: 10,
                labels: {
                    formatter: function(val) { return val.toFixed(1); },
                    style: { colors: '#84919A' }
                },
                axisBorder: { color: '#84919A' },
                axisTicks: { color: '#84919A' }
            },
            yaxis: [
                {
                    opposite: true,
                    title: { text: 'Temp (°F)' },
                    axisTicks: { show: true },
                    axisBorder: { show: true, color: '#FFCB4E' },
                    labels: { style: { colors: '#FFCB4E' }, formatter: function(val) { return val.toFixed(0); } },
                    min: 150, max: 210
                },
                {
                    opposite: true,
                    title: { text: 'Pressure' },
                    axisTicks: { show: true },
                    axisBorder: { show: true, color: '#00AEF5' },
                    labels: { style: { colors: '#00AEF5' }, formatter: function(val) { return val.toFixed(1); } },
                    min: 0, max: 11
                },
                {
                    opposite: true,
                    title: { text: 'Flow (ml/s)' },
                    labels: { style: { colors: '#888888' }, formatter: function(val) { return val.toFixed(1); } },
                    min: 0, max: 10
                },
                {
                    title: { text: 'Weight' },
                    axisTicks: { show: true },
                    axisBorder: { show: true, color: '#4EE29B' },
                    labels: { style: { colors: '#4EE29B' }, formatter: function(val) { return val.toFixed(1); } },
                    min: 0, max: 50
                },
                {
                    opposite: true,
                    title: { text: 'PID (%)' },
                    labels: { style: { colors: '#993b3b' }, formatter: function(val) { return val.toFixed(0); } },
                    min: 0, max: 100
                }
            ],
            legend: {
                position: 'top',
                horizontalAlign: 'left'
            },
            grid: {
                borderColor: '#252525'
            },
            theme: { mode: 'dark' },
            tooltip: {
                shared: true,
                intersect: false,
                x: {
                    formatter: function(val) { return val + ' seconds'; }
                }
            },
            responsive: [{
                breakpoint: 1000,
                options: {
                    legend: { position: 'bottom' }
                }
            }]
        };

        chart = new ApexCharts(document.querySelector('#chart'), options);
        chart.render();
        chartInitialized = true;
    }

    // WebSocket connection
    function connectWebSocket() {
        var wsUrl = 'ws://' + window.location.hostname + ':81/';
        ws = new WebSocket(wsUrl);

        ws.onopen = function() {
            console.log('WebSocket connected');
            wsReconnectAttempts = 0;
        };

        ws.onmessage = function(event) {
            try {
                var data = JSON.parse(event.data);
                updateUIFromWebSocket(data);
            } catch (e) {
                console.error('WebSocket parse error:', e);
            }
        };

        ws.onclose = function() {
            console.log('WebSocket disconnected');
            if (wsReconnectAttempts < maxReconnectAttempts) {
                wsReconnectAttempts++;
                setTimeout(connectWebSocket, 2000);
            }
        };

        ws.onerror = function(error) {
            console.error('WebSocket error:', error);
        };
    }

    // Update UI from WebSocket data
    function updateUIFromWebSocket(data) {
        // Update metric displays
        if (data.currentTemp !== undefined) {
            document.getElementById('currentTemp').innerHTML = data.currentTemp.toFixed(1) + ' <span class="fw-light fs-6 fst-italic">°F</span>';
        }
        if (data.currentPressure !== undefined) {
            document.getElementById('currentPressure').innerHTML = data.currentPressure.toFixed(1) + ' <span class="fw-light fs-6 fst-italic">BAR</span>';
        }
        if (data.pidOutput !== undefined) {
            document.getElementById('pidOutput').innerHTML = data.pidOutput.toFixed(0) + ' <span class="fw-light fs-6 fst-italic">%</span>';
        }
        if (data.shotWeight !== undefined) {
            document.getElementById('shotWeight').innerHTML = data.shotWeight.toFixed(1) + ' <span class="fw-light fs-6 fst-italic">g</span>';
        }
        if (data.shotTime !== undefined) {
            document.getElementById('shotTime').innerHTML = data.shotTime.toFixed(1) + ' <span class="fw-light fs-6 fst-italic">s</span>';
        }
        if (data.flowRate !== undefined) {
            document.getElementById('flowRate').innerHTML = data.flowRate.toFixed(2) + ' <span class="fw-light fs-6 fst-italic">ml/s</span>';
        }
        if (data.doseWeight !== undefined) {
            document.getElementById('doseWeightDisplay').innerHTML = data.doseWeight.toFixed(1) + ' <span class="fw-light fs-6 fst-italic">g</span>';
        }
        if (data.systemState !== undefined) {
            document.getElementById('systemState').innerHTML = '<span class="fw-light fs-6 fst-italic">' + data.systemState + '</span>';
            
            // Update cleaning buttons visibility
            var isCleaning = (data.systemState === 'CLEANING');
            document.getElementById('startCleaningBtn').style.display = isCleaning ? 'none' : 'block';
            document.getElementById('stopCleaningBtn').style.display = isCleaning ? 'block' : 'none';
        }

        // Calculate and update shot ratio
        if (data.doseWeight !== undefined && data.shotWeight !== undefined && data.doseWeight > 0) {
            var ratio = data.shotWeight / data.doseWeight;
            document.getElementById('shotRatio').innerHTML = ratio.toFixed(1) + ':1';
        }

        // Handle first-run setup banner
        if (data.firstRun !== undefined) {
            var banner = document.getElementById('firstRunBanner');
            if (data.firstRun) {
                banner.classList.remove('d-none');
                
                // Update calibration status indicators
                if (data.tempCalComplete !== undefined) {
                    document.getElementById('tempCalStatus').textContent = data.tempCalComplete ? '✅ Temperature Probe' : '❌ Temperature Probe';
                }
                if (data.pressureCalComplete !== undefined) {
                    document.getElementById('pressureCalStatus').textContent = data.pressureCalComplete ? '✅ Pressure Transducer' : '❌ Pressure Transducer';
                }
            } else {
                banner.classList.add('d-none');
            }
        }

        // Handle error/warning messages
        if (data.errorMessage !== undefined && data.errorMessage !== '') {
            var errorBanner = document.getElementById('errorBanner');
            document.getElementById('errorText').textContent = data.errorMessage;
            errorBanner.classList.remove('d-none');
        } else {
            document.getElementById('errorBanner').classList.add('d-none');
        }

        // Update chart during brew
        if (data.isBrewing && chartInitialized) {
            var now = Date.now();
            if (startTime === null) {
                startTime = now;
                tempData = [];
                pressureData = [];
                flowData = [];
                weightData = [];
                pidData = [];
            }

            var elapsed = (now - startTime) / 1000;
            tempData.push({ x: elapsed, y: data.currentTemp });
            pressureData.push({ x: elapsed, y: data.currentPressure });
            flowData.push({ x: elapsed, y: data.flowRate });
            weightData.push({ x: elapsed, y: data.shotWeight });
            pidData.push({ x: elapsed, y: data.pidOutput });

            // Limit data points
            var maxPoints = 300;
            if (tempData.length > maxPoints) {
                tempData.shift();
                pressureData.shift();
                flowData.shift();
                weightData.shift();
                pidData.shift();
            }

            chart.updateSeries([
                { name: 'Temperature (°F)', data: tempData },
                { name: 'Pressure (BAR)', data: pressureData },
                { name: 'Flow Rate (ml/s)', data: flowData },
                { name: 'Weight (g)', data: weightData },
                { name: 'PID Output (%)', data: pidData }
            ]);
        } else if (!data.isBrewing) {
            startTime = null;
        }
    }

    // Fetch status via REST API (fallback)
    async function updateStatus() {
        try {
            var response = await fetch('/status');
            var data = await response.json();

            // Populate form fields with current values
            if (data.brewTemp) document.getElementById('brewTemp').value = data.brewTemp;
            if (data.shotTargetTime) document.getElementById('shotTargetTime').value = data.shotTargetTime;
            if (data.doseWeight) document.getElementById('doseWeightInput').value = data.doseWeight;
            if (data.shotTargetWeight) document.getElementById('shotTargetWeight').value = data.shotTargetWeight;
            if (data.pidKp) document.getElementById('pidKp').value = data.pidKp;
            if (data.pidKi) document.getElementById('pidKi').value = data.pidKi;
            if (data.pidKd) document.getElementById('pidKd').value = data.pidKd;
            if (data.pidBoost) document.getElementById('pidBoost').value = data.pidBoost;
            if (data.preInfusion) document.getElementById('preInfusion').value = data.preInfusion;
            if (data.preInfusionRate) document.getElementById('preInfusionRate').value = data.preInfusionRate;

        } catch (error) {
            console.error('Failed to fetch status:', error);
        }
    }

    // Update shot parameters
    async function updateParameters() {
        try {
            var params = new URLSearchParams();
            params.append('brewTemp', document.getElementById('brewTemp').value);
            params.append('shotTargetTime', document.getElementById('shotTargetTime').value);
            params.append('doseWeight', document.getElementById('doseWeightInput').value);
            params.append('shotTargetWeight', document.getElementById('shotTargetWeight').value);

            var response = await fetch('/setParams', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: params
            });

            if (response.ok) {
                showMessage('Parameters saved!', 'success');
            } else {
                showMessage('Failed to save', 'error');
            }
        } catch (error) {
            showMessage('Error: ' + error.message, 'error');
        }
    }

    // Update advanced parameters
    async function updateAdvancedParameters() {
        try {
            var params = new URLSearchParams();
            params.append('kp', document.getElementById('pidKp').value);
            params.append('ki', document.getElementById('pidKi').value);
            params.append('kd', document.getElementById('pidKd').value);
            params.append('boost', document.getElementById('pidBoost').value);
            params.append('preinfusion', document.getElementById('preInfusion').value);
            params.append('rate', document.getElementById('preInfusionRate').value);

            var response = await fetch('/setPID', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: params
            });

            if (response.ok) {
                showAdvancedMessage('Advanced settings saved!', 'success');
            } else {
                showAdvancedMessage('Failed to save', 'error');
            }
        } catch (error) {
            showAdvancedMessage('Error: ' + error.message, 'error');
        }
    }

    // Tare scale
    async function tareScale() {
        try {
            var response = await fetch('/tare', { method: 'POST' });
            if (response.ok) {
                showMessage('Scale tared!', 'success');
            }
        } catch (error) {
            console.error('Tare failed:', error);
        }
    }

    // Cleaning cycle controls
    async function startCleaning() {
        try {
            var response = await fetch('/cleaning/start', { method: 'POST' });
            if (response.ok) {
                showMessage('Cleaning cycle started', 'success');
            }
        } catch (error) {
            console.error('Start cleaning failed:', error);
        }
    }

    async function stopCleaning() {
        try {
            var response = await fetch('/cleaning/stop', { method: 'POST' });
            if (response.ok) {
                showMessage('Cleaning cycle stopped', 'success');
            }
        } catch (error) {
            console.error('Stop cleaning failed:', error);
        }
    }

    // Calibration functions
    async function startFillCalibration() {
        try {
            var response = await fetch('/calibrate/fill/start', { method: 'POST' });
            if (response.ok) {
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Fill calibration start failed:', error);
        }
    }

    async function nextCalibrationStep() {
        try {
            var response = await fetch('/calibrate/fill/next', { method: 'POST' });
            if (response.ok) {
                updateCalibrationStatus();
            } else {
                var text = await response.text();
                showMessage(text, 'error');
            }
        } catch (error) {
            console.error('Next calibration step failed:', error);
        }
    }

    async function startScaleCalibration() {
        try {
            var response = await fetch('/calibrate/scale/start', { method: 'POST' });
            if (response.ok) {
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Scale calibration start failed:', error);
        }
    }

    async function completeScaleCalibration() {
        try {
            var knownWeight = document.getElementById('knownWeight').value;
            var response = await fetch('/calibrate/scale/complete?weight=' + knownWeight, { method: 'POST' });
            if (response.ok) {
                showMessage('Scale calibration complete!', 'success');
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Scale calibration complete failed:', error);
        }
    }

    async function startPressureCalibration() {
        try {
            var response = await fetch('/calibrate/pressure/start', { method: 'POST' });
            if (response.ok) {
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Pressure calibration start failed:', error);
        }
    }

    async function nextPressureCalibrationStep() {
        try {
            var response = await fetch('/calibrate/pressure/next', { method: 'POST' });
            if (response.ok) {
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Next pressure calibration step failed:', error);
        }
    }

    async function startCalibrationPump() {
        try {
            var response = await fetch('/calibrate/pressure/pump/start', { method: 'POST' });
            if (response.ok) {
                showMessage('Pump started', 'success');
            }
        } catch (error) {
            console.error('Start pump failed:', error);
        }
    }

    async function stopCalibrationPump() {
        try {
            var response = await fetch('/calibrate/pressure/pump/stop', { method: 'POST' });
            if (response.ok) {
                showMessage('Pump stopped', 'success');
            }
        } catch (error) {
            console.error('Stop pump failed:', error);
        }
    }

    async function setPressureCalibrationValue() {
        try {
            var actualPressure = document.getElementById('actualPressure').value;
            var response = await fetch('/calibrate/pressure/set?pressure=' + actualPressure, { method: 'POST' });
            if (response.ok) {
                showMessage('Pressure calibration value set!', 'success');
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Set pressure value failed:', error);
        }
    }

    // Temperature probe manual confirmation
    async function confirmTempProbe() {
        try {
            var response = await fetch('/calibrate/temp/complete', { method: 'POST' });
            if (response.ok) {
                var data = await response.json();
                showMessage('Temp probe confirmed at ' + data.temperature.toFixed(1) + '°F', 'success');
                updateCalibrationStatus();
            } else {
                var errData = await response.json();
                showMessage('Temp probe error: ' + (errData.error || 'invalid reading'), 'error');
            }
        } catch (error) {
            console.error('Temp probe confirmation failed:', error);
        }
    }

    // Cancel any active calibration
    async function cancelCalibration() {
        try {
            var response = await fetch('/calibrate/cancel', { method: 'POST' });
            if (response.ok) {
                showMessage('Calibration cancelled', 'success');
                updateCalibrationStatus();
            }
        } catch (error) {
            console.error('Cancel calibration failed:', error);
        }
    }

    async function updateCalibrationStatus() {
        try {
            var response = await fetch('/calibrate/status');
            var data = await response.json();
            var statusDiv = document.getElementById('calibrationStatus');

            if (data.active || data.pressureCalActive) {
                var calType = data.active ? 'Fill/Scale' : 'Pressure';
                var stepName = data.active ? data.step : data.pressureCalStep;
                statusDiv.innerHTML = '<strong>' + calType + ' Calibration Active:</strong> ' + stepName + '<br>' +
                                      '<strong>Instructions:</strong> ' + data.instructions;
            } else {
                statusDiv.innerHTML = '<em>No active calibration</em>';
            }
        } catch (error) {
            console.error('Failed to update calibration status:', error);
        }
    }

    // Show message
    function showMessage(text, type) {
        var msgDiv = document.getElementById('message');
        msgDiv.textContent = text;
        msgDiv.className = type;
        setTimeout(function() {
            msgDiv.textContent = '';
            msgDiv.className = '';
        }, 3000);
    }

    // Show advanced message
    function showAdvancedMessage(text, type) {
        var msgDiv = document.getElementById('advancedMessage');
        msgDiv.textContent = text;
        msgDiv.className = 'alert ' + (type === 'success' ? 'alert-success' : 'alert-danger');
        setTimeout(function() {
            msgDiv.textContent = '';
            msgDiv.className = 'alert alert-danger d-none';
        }, 5000);
    }

    // Handle window resize for responsive chart updates
    function handleResize() {
        if (chartInitialized && chart) {
            var isMobile = window.innerWidth < 768;

            chart.updateOptions({
                chart: {
                    width: '100%',
                    height: isMobile ? 300 : 500
                }
            });
        }
    }

    // Initialize
    document.addEventListener('DOMContentLoaded', function() {
        initChart();
        updateStatus();
        updateCalibrationStatus();

        // Initialize WebSocket connection
        connectWebSocket();
        setInterval(updateCalibrationStatus, 2000);

        // Add resize listener for responsive chart updates
        window.addEventListener('resize', handleResize);
    });
)rawliteral";

#endif // WEB_UI_JS_H
