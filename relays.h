// relays.h — the 4 physical relay outputs and their safety interlocks.
// Core 0 calls these from state_machine.cpp/web_api.cpp; control_task.cpp
// (Core 1) calls set_boiler_element()/set_pump() directly during a critical
// error, exactly as before.
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
