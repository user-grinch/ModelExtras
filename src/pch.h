#pragma once

#include <regex>
#include "CBike.h"
#include "CAutomobile.h"
#include "CTimer.h"
#include "NodeName.h"
#include "CBike.h"
#include "CAutomobile.h"
#include "CModelInfo.h"
#include "log.h"

using namespace plugin;

#include "Common.h"

extern VehicleExtendedData<FCData> vehdata;

#define MOD_DATA_PATH(x) PLUGIN_PATH((char*)("FunctionalComponents/"x))
#define MOD_DATA_PATH_S(x) PLUGIN_PATH((char*)("FunctionalComponents/" + x).c_str())