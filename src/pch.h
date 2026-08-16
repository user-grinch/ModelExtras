#pragma once
#include <CTimer.h>
#include <NodeName.h>
#include <CModelInfo.h>

#include <RenderWare.h>
#include <plugin.h>
#include <game_sa/common.h>

#include "nlohmann/json.hpp"
#include "db/ini.hpp"
#include "AixLog/AixLog.hpp"
#include "vkeys.h"
#include <format>
#include "utils/util.h"

using namespace plugin;

extern CIniReader gConfig;
extern bool gVerboseLogging;

#define LOG_NO_LEVEL(x) LOG(INFO) << x;
#define LOG_VERBOSE(fmt, ...)             \
  do                                      \
  {                                       \
    if (gVerboseLogging)                  \
    {                                     \
      LOG(DEBUG) << std::format(fmt, ##__VA_ARGS__); \
    }                                     \
  } while (0)


static inline CBaseModelInfo **CModelInfo__ms_modelInfoPtrs = reinterpret_cast<CBaseModelInfo **>(patch::GetPointer(0x403DA7));