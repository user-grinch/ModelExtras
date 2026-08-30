#pragma once
#include <plugin.h>

#define MOD_DATA_PATH(x) PLUGIN_PATH((char *)("ModelExtras/" x))

#define MOD_NAME "ModelExtras"
#define MOD_VERSION "3.0"
#define MOD_VERSION_NUMBER 30000
#define MOD_VERSION_SUFFIX ""

#define MOD_TITLE MOD_NAME " v" MOD_VERSION

#define DISCORD_INVITE "https://discord.gg/AduJVdyqCD"
#define GITHUB_LINK "https://github.com/user-grinch/ModelExtras"

extern bool gbProperShadersDetected;

#define STR_FOUND(x, y) ((x).find(y) != std::string::npos)