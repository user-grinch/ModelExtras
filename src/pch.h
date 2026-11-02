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

// Dynamic model limit support for Fastman92 Limit Adjuster, Open Limit Adjuster, etc.
// In vanilla GTA SA, the engine limit is 20000. With limit adjusters, model IDs span dynamically.
inline int GetMaxModelCount()
{
    int count = CModelInfo::ms_modelInfoCount;
    return count > 20000 ? count : 20000;
}

inline bool IsValidModelIndex(int model)
{
    return model > 0 && model < GetMaxModelCount();
}

inline CBaseModelInfo *GetSafeModelInfo(int model)
{
    return (IsValidModelIndex(model) && CModelInfo::ms_modelInfoPtrs) ? CModelInfo::GetModelInfo(model) : nullptr;
}