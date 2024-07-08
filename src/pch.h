#pragma once
#include <CBike.h>
#include <CAutomobile.h>
#include <CTimer.h>
#include <NodeName.h>
#include <CModelInfo.h>
#include <regex>

#include <plugin.h>

#include "bass.h"
#include "json.hpp"
#include "ini.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
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

#define MOD_DATA_PATH(x) PLUGIN_PATH((char*)("ModelExtras/"x))
#define MOD_DATA_PATH_S(x) PLUGIN_PATH((char*)("ModelExtras/" + x).c_str())

#define MOD_NAME "ModelExtras"
#define MOD_VERSION_NUMBER "1.6"
#define MOD_TITLE MOD_NAME " v" MOD_VERSION_NUMBER"-beta"
#define DISCORD_INVITE "https://discord.gg/ZzW7kmf"
#define GITHUB_LINK "https://github.com/user-grinch/ModelExtras"
#define PATREON_LINK "https://www.patreon.com/grinch_"

#define NODE_FOUND(x, y) x.find(y) != std::string::npos
#define NODE_NOT_FOUND(x, y) x.find(y) == std::string::npos

extern std::shared_ptr<spdlog::logger> gLogger;
extern CIniReader gConfig;
static inline CdeclEvent <AddressList<0x5E7859, H_CALL>, PRIORITY_BEFORE, ArgPickN<CPed*, 0>, void(CPed*)> weaponRenderEvent;
static inline ThiscallEvent <AddressList<//0x43D821, H_CALL,
                               //   0x43D939, H_CALL,
                                  0x45CC78, H_CALL,
                                  0x47D3AD, H_CALL,
                                //   0x5E3B53, H_CALL,
                                //   0x5E5F14, H_CALL,
                                //   0x5E6150, H_CALL,
                                //   0x5E6223, H_CALL,
                                  0x5E6327, H_CALL,
                                  0x63072E, H_CALL,
                                //   0x5E6483, H_CALL,
                                  0x6348FC, H_CALL>, PRIORITY_BEFORE, ArgPick2N<CPed*, 0, int, 1>, void(CPed*, int)> weaponRemoveEvent;
static CBaseModelInfo ** CModelInfo__ms_modelInfoPtrs = reinterpret_cast<CBaseModelInfo **>(plugin::patch::GetPointer(0x403DA7));
#define PRINT_LINEBREAK \
    gLogger->set_pattern("%v"); \
    gLogger->info(""); \
    gLogger->set_pattern("[%L] %v");