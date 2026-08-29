#pragma once
#include <CTimer.h>
#include <NodeName.h>
#include <game_sa/CModelInfo.h>

#include <RenderWare.h>
#include <plugin.h>
#include <game_sa/common.h>

#include "nlohmann/json.hpp"
#include "ini/ini.hpp"
#include "AixLog/AixLog.hpp"
#include <format>
#include "utils/util.h"
#include "vkeys.h"

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


inline std::string_view GetSafeFrameNodeName(RwFrame *pFrame)
{
    if (!pFrame) return {};
    const char *name = GetFrameNodeName(pFrame);
    return name ? std::string_view(name) : std::string_view{};
}

static inline CBaseModelInfo **CModelInfo__ms_modelInfoPtrs = reinterpret_cast<CBaseModelInfo **>(patch::GetPointer(0x403DA7));