#pragma once
#include "plugin.h"
#include "Common.h"

void ProcessGearMeter(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);
void ProcessOdoMeter(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);
void ProcessRPMMeter(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);
void ProcessSpeedoMeter(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);