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

enum class eNodeEntityType {
    Ped,
    Object,
    Vehicle,
    Weapon,
};

#define MOD_DATA_PATH(x) PLUGIN_PATH((char*)("ModelExtras/"x))
#define MOD_DATA_PATH_S(x) PLUGIN_PATH((char*)("ModelExtras/" + x).c_str())

#define MOD_NAME "ModelExtras"
#define MOD_VERSION_NUMBER "1.2"
#define MOD_VERSION MOD_VERSION_NUMBER
#define MOD_TITLE MOD_NAME " v" MOD_VERSION
#define DISCORD_INVITE "https://discord.gg/ZzW7kmf"
#define GITHUB_LINK "https://github.com/user-grinch/ModelExtras"
#define PATREON_LINK "https://www.patreon.com/grinch_"