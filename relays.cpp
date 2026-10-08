// relays.cpp — physical relay outputs + safety interlocks. Extracted
// verbatim from the original set_pump()/set_brew_solenoid()/
// set_fill_solenoid()/set_boiler_element(), with one addition: each now
// returns whether the command was applied or refused by an interlock
// (previously logged to Serial only, with no way for a caller to react).
// The dead "legacy" set_ssr() wrapper (zero callers) has been removed.
#include <Arduino.h>
#include "config.h"
#include "shared_state.h"
#include "relays.h"

// brewActive/fillActive: owned by state_machine (main .ino).
// pressureLockoutActive/currentPressure: owned by sensors (main .ino for now).
extern bool brewActive;
extern bool fillActive;
extern bool pressureLockoutActive;
extern double currentPressure;

void initRelays() {
  pinMode(PIN_RELAY_CH1, OUTPUT);
  digitalWrite(PIN_RELAY_CH1, LOW);  // Brew Solenoid OFF
  pinMode(PIN_RELAY_CH2, OUTPUT);
  digitalWrite(PIN_RELAY_CH2, LOW);  // Pump OFF
  pinMode(PIN_RELAY_CH3, OUTPUT);
  digitalWrite(PIN_RELAY_CH3, LOW);  // Fill Solenoid OFF
  pinMode(PIN_RELAY_CH4, OUTPUT);
  digitalWrite(PIN_RELAY_CH4, LOW);  // Boiler Element SSR OFF
  Serial.println("4-Channel relay system initialized");
}

bool set_brew_solenoid(bool on) {
  // Safety interlock: prevent brew solenoid activation during fill
  if (on && fillActive) {
    Serial.println("INTERLOCK: Brew solenoid blocked - fill active");
    return false;
  }
  digitalWrite(PIN_RELAY_CH1, on ? HIGH : LOW);
  return true;
}

bool set_pump(bool on) {
  // Pressure safety lockout - prevent pump activation during over-pressure
  if (on && pressureLockoutActive) {
    Serial.printf("PRESSURE LOCKOUT: Pump activation blocked - pressure %.1f BAR (lockout until ≤%.1f BAR)\n", currentPressure, PRESSURE_LOCKOUT_LOW);
    return false;  // Pump stays OFF until pressure drops to 11.0 BAR or below
  }
  digitalWrite(PIN_RELAY_CH2, on ? HIGH : LOW);
  return true;
}

bool set_fill_solenoid(bool on) {
  // Safety interlock: prevent fill solenoid activation during brew
  if (on && brewActive) {
    Serial.println("INTERLOCK: Fill solenoid blocked - brew active");
    return false;
  }
  digitalWrite(PIN_RELAY_CH3, on ? HIGH : LOW);
  return true;
}

bool set_boiler_element(bool on) {
#ifdef BENCH_MODE
  // Bench mode: block heater relay, keep it OFF regardless of PID request
  digitalWrite(PIN_RELAY_CH4, LOW);
  setBoilerHeatingState(false);
  return !on;  // "refused" if caller wanted it on, matching original BENCH_MODE intent
#else
  digitalWrite(PIN_RELAY_CH4, on ? HIGH : LOW);
  setBoilerHeatingState(on);
  return true;
#endif
}
