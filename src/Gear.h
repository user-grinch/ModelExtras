#pragma once
#include "plugin.h"
#include "Common.h"

void ProcessClutch(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);
void ProcessGearLever(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);
void ProcessGearSound(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);