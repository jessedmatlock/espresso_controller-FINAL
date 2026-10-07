# Espresso Controller Refactoring Plan

## Overview
Post-fix simplification pass targeting ~650 lines of reduction while improving
maintainability and eliminating recurring bug patterns. No functional changes.

## Prerequisites
- All 17 Phase 2 fixes applied and verified
- Working hardware test after fix deployment

---

## R1: Unified JSON Builder (~200 lines saved)

**Current**: handleStatus() (190 lines, 24 snprintf, 24 static lastSent* vars),
sendWebSocketData() (12 lines), broadcastWebSocketData() (20 lines) all build
near-identical JSON independently. handleStatus() has delta compression that
saves negligible bandwidth on a local WiFi AP.

**Target**: One function: `buildStatusJSON(char* buf, size_t len, bool includeSetup)`

**Steps**:
1. Create `buildStatusJSON()` that writes full status to provided buffer
2. Replace handleStatus() body: call builder, send result
3. Replace sendWebSocketData() body: call builder, send via WS
4. Replace broadcastWebSocketData() body: call builder, broadcast via WS
5. Delete 24 static `lastSent*` variables and `firstRequest` flag
6. Delete delta-compression comparison logic

**Risk**: Low. Output format unchanged. WebSocket clients already receive full updates.

**Verification**: Compare JSON output before/after with web UI connected.

---

## R2: Table-Driven Parameter Handlers (~120 lines saved)

**Current**: handleSetpoint(), handleSetDoseWeight(), handleSetPIDParameters()
each manually parse args, validate range, assign, save EEPROM, respond.
6 parameters in handleSetPIDParameters alone follow identical pattern.

**Target**: Parameter definition table + single generic handler.

**Steps**:
1. Define struct:
   ```cpp
   struct ParamDef {
     const char* argName;
     double* target;
     double min;
     double max;
     const char* label;
   };
   ```
2. Create table of all configurable parameters (8-10 entries)
3. Write `handleSetParam()` that iterates table, validates, assigns, saves
4. Register single `/setParam` endpoint (or keep existing URLs routing to shared handler)
5. Remove individual handler functions
6. Update web UI JS to use new endpoint (if URL changes)

**Risk**: Medium. Web UI JavaScript must match endpoint. Test all parameters via UI.

**Verification**: Set each parameter via web UI, verify EEPROM save, verify value reflected in status.

---

## R3: Unified Calibration State Machine (~120 lines saved)

**Current**: 3 separate state machines (fill, scale, pressure) with separate
variables, separate String-based step tracking, separate start/process/complete functions.

**Target**: Single calibration framework with enum-based state.

**Steps**:
1. Define enums:
   ```cpp
   enum CalType { CAL_NONE, CAL_FILL, CAL_SCALE, CAL_PRESSURE, CAL_TEMP };
   enum CalStep { CAL_IDLE, CAL_STEP_1, CAL_STEP_2, CAL_STEP_3, CAL_COMPLETE };
   ```
2. Replace `calibrationMode`/`calibrationStep` (String) and
   `pressureCalibrationMode`/`pressureCalibrationStep` (String) with enums
3. Single `startCalibration(CalType type)` function
4. Single `processCalibrationStep()` with switch on CalType then CalStep
5. Single `completeCalibration()` dispatcher
6. Remove 2 global String variables

**Risk**: Medium. Calibration is infrequently used but must work correctly.
Test each calibration type end-to-end after refactor.

**Verification**: Run fill probe, scale, pressure, and temperature calibration
through web UI. Verify EEPROM persistence and setupComplete flag.

---

## R4: Reduce Mutex Count with Atomics (~60 lines saved)

**Current**: 8 mutex types, 50 critical section pairs.

**Target**: 4-5 mutexes by using atomics where appropriate.

**Steps**:
1. Replace `errorFlagMux` + `volatile uint8_t errorFlags` with
   `std::atomic<uint8_t> errorFlags`. Remove 6 critical section pairs.
2. Merge `tempMux` and `cachedTempMux` into single `tempMux` (both protect
   single doubles, same access pattern)
3. Audit `controlMux` usage — identify display-only reads where torn read
   is harmless (bool reads for OLED icons). Remove unnecessary mutex for
   those reads.
4. Keep `pidMux`, `stateMux`, `scaleMux`, merged `tempMux`, `controlMux`

**Risk**: Medium-High. Thread safety analysis required for each removal.
Must verify no torn reads affect safety-critical decisions.

**Verification**: Run PID under load with brew cycles. Monitor for jitter
warnings. Verify temperature display stability.

---

## R5: Consolidated Error System (~50 lines saved)

**Current**: Error handling requires calling `setErrorFlag()` +
`updateSystemMessage()` + `handleErrorActions()` at each error site.
36 call sites total. Easy to forget one step.

**Target**: Single `setError(uint8_t flag)` and `clearError(uint8_t flag)`.

**Steps**:
1. Create `setError(uint8_t flag)` that atomically:
   - Sets the error flag
   - Derives system message from flags
   - Triggers appropriate actions (emergency stop for critical, display for non-critical)
2. Create `clearError(uint8_t flag)` that atomically:
   - Clears the flag
   - Re-derives system message
   - Handles recovery actions
3. Replace all `setErrorFlag()` + `updateSystemMessage()` + `handleErrorActions()`
   call sequences with single `setError()` call
4. Remove standalone `updateSystemMessage()` and `handleErrorActions()` functions

**Risk**: Medium. Must verify all error paths still trigger correct actions.

**Verification**: Induce each error type (temp disconnect, over-temp, scale
disconnect, pressure over-limit) and verify correct message, state change,
and recovery behavior.

---

## R6: Deferred EEPROM Save (~30 lines saved at call sites)

**Current**: 18 call sites invoke `saveParametersToEEPROM()` directly.
Each triggers a flash write. Flash has limited write cycles (~100K).

**Target**: Dirty-flag pattern with periodic save.

**Steps**:
1. Add `volatile bool eepromDirty = false;`
2. Replace all 18 `saveParametersToEEPROM()` calls with `eepromDirty = true;`
3. In `handleSystemMonitoring()`, add:
   ```cpp
   static unsigned long lastEepromSave = 0;
   if (eepromDirty && millisElapsed(lastEepromSave, 2000)) {
     saveParametersToEEPROM();
     eepromDirty = false;
     lastEepromSave = millis();
   }
   ```
4. Also save on state transitions to IDLE (catch parameter changes before power-off)
5. Keep direct save in `setup()` for first-run initialization

**Risk**: Low-Medium. Parameters could be lost if power cut within 2s of change.
Acceptable for espresso machine (user is present and aware).

**Verification**: Change parameters via web UI, verify EEPROM updates within
2 seconds. Power cycle and verify persistence.

---

## Implementation Order (recommended)

1. **R6** (Deferred EEPROM) — simplest, immediate flash wear benefit
2. **R1** (JSON builder) — largest line savings, no logic change
3. **R5** (Error system) — reduces bug surface for future changes
4. **R2** (Table-driven params) — moderate savings, clean pattern
5. **R3** (Calibration unification) — moderate savings, careful testing needed
6. **R4** (Mutex reduction) — highest risk, do last with thorough testing

## Estimated Total Savings
~650 lines removed, 3 String globals eliminated, 13 fewer static variables,
reduced flash wear, simplified maintenance.
