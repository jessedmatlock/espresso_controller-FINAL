// relays.h — the 4 physical relay outputs and their safety interlocks.
// Called from state_machine.cpp/web_api.cpp; pid_control.cpp also calls
// set_boiler_element()/set_pump() directly during a critical error.
//
// Each setter now returns whether the command was actually applied (true)
// or silently refused by an interlock (false) — callers that don't check it
// behave exactly as they did before this was added.
#pragma once

void initRelays();
bool set_brew_solenoid(bool on);
bool set_pump(bool on);
bool set_fill_solenoid(bool on);
bool set_boiler_element(bool on);
