#pragma once
#include <CModelInfo.h>
#include <CTimer.h>
#include <NodeName.h>

#include <RenderWare.h>
#include <plugin.h>

#include "ini.hpp"
#include "json.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "util.h"
#include "vkeys.h"

using namespace plugin;

enum class eModelEntityType {
  Ped,
  Object,
  Vehicle,
  Weapon,
  Jetpack,
};

extern std::shared_ptr<spdlog::logger> gLogger;
extern CIniReader gConfig;

#define LOG_NO_LEVEL(x)                                                        \
  gLogger->set_pattern("%v");                                                  \
  gLogger->info(x);                                                            \
  gLogger->set_pattern("[%L] %v");

extern bool gVerboseLogging;

#define LOG_VERBOSE(fmt, ...)                                                  \
  if (gVerboseLogging) {                                                       \
    gLogger->debug(fmt, __VA_ARGS__);                                          \
  }

extern unsigned int FramePluginOffset;
#define PLUGIN_ID_STR 'MEX'
#define PLUGIN_ID_NUM 0x42945628
#define FRAME_EXTENSION(frame)                                                 \
  ((RwFrameExtension *)((unsigned int)frame + FramePluginOffset))

struct RwFrameExtension {
  CVehicle *pOwner;
  RwMatrix *pOrigMatrix;

  static RwFrame *Initialize(RwFrame *pFrame) {
    FRAME_EXTENSION(pFrame)->pOwner = nullptr;
    FRAME_EXTENSION(pFrame)->pOrigMatrix = nullptr;
    return pFrame;
  }

  static RwFrame *Shutdown(RwFrame *pFrame) {
    if (FRAME_EXTENSION(pFrame)->pOrigMatrix) {
      delete FRAME_EXTENSION(pFrame)->pOrigMatrix;
    }
    return pFrame;
  }

  static RwFrame *Clone(RwFrame *pCopy, RwFrame *pFrame) { return pCopy; }
};

static inline CBaseModelInfo **CModelInfo__ms_modelInfoPtrs =
    reinterpret_cast<CBaseModelInfo **>(plugin::patch::GetPointer(0x403DA7));