#pragma once
#include "plugin.h"
#include "Common.h"

void ProcessFrontBrake(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);
void ProcessRearBrake(const std::string& name, RwFrame* frame, FCData& data, CVehicle* pVeh);