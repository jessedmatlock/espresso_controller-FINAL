// eeprom_store.h — EEPROM persistence for all configurable parameters.
// Core 0 only.
#pragma once

void initEEPROM();
void saveParametersToEEPROM();
bool loadParametersFromEEPROM();
