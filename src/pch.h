#pragma once
#include <CAutomobile.h>
#include <CBike.h>
#include <CTimer.h>
#include <NodeName.h>
#include <CModelInfo.h>
#include <regex>

#include <RenderWare.h>
#include <plugin.h>

#include "json.hpp"
#include "ini.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "util.h"
#include "vkeys.h"

using namespace plugin;

enum class eModelEntityType
{
  Ped,
  Object,
  Vehicle,
  Weapon,
  Jetpack,
};

extern std::shared_ptr<spdlog::logger> gLogger;
extern CIniReader gConfig;

#define LOG_NO_LEVEL(x)       \
  gLogger->set_pattern("%v"); \
  gLogger->info(x);           \
  gLogger->set_pattern("[%L] %v");

extern bool gVerboseLogging;

#define LOG_VERBOSE(fmt, ...)         \
  if (gVerboseLogging)                \
  {                                   \
    gLogger->debug(fmt, __VA_ARGS__); \
  }

static inline CBaseModelInfo **CModelInfo__ms_modelInfoPtrs = reinterpret_cast<CBaseModelInfo **>(plugin::patch::GetPointer(0x403DA7));