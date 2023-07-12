#pragma once

#include <CBike.h>
#include <CAutomobile.h>
#include <CTimer.h>
#include <NodeName.h>
#include <CModelInfo.h>

#include "bass.h"

#include "log.h"
#include "util.h"

using namespace plugin;

#define MOD_DATA_PATH(x) PLUGIN_PATH((char*)("FunctionalComponents/"x))
#define MOD_DATA_PATH_S(x) PLUGIN_PATH((char*)("FunctionalComponents/" + x).c_str())