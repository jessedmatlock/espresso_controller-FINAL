// eeprom_store.h — EEPROM persistence for all configurable parameters.
// Single task only.
#pragma once

void initEEPROM();
void saveParametersToEEPROM();
bool loadParametersFromEEPROM();
