#pragma once
#include <plugin.h>

#define CORONA_SZ_MUL 0.7f
#define AMBIENT_ON_VAL 8.0f

#define MOD_DATA_PATH(x) PLUGIN_PATH((char *)("ModelExtras/" x))
#define MOD_DATA_PATH_S(x) PLUGIN_PATH((char *)("ModelExtras/" + x).c_str())

#define MOD_NAME "ModelExtras"
#define MOD_VERSION "2.0"
#define MOD_VERSION_NUMBER 20000
#define MOD_TITLE MOD_NAME " v" MOD_VERSION "-rc12"
#define DISCORD_INVITE "https://discord.gg/AduJVdyqCD"
#define GITHUB_LINK "https://github.com/user-grinch/ModelExtras"
#define PATREON_LINK "https://www.patreon.com/grinch_"

#define STR_FOUND(x, y) x.find(y) != std::string::npos
#define STR_NOT_FOUND(x, y) x.find(y) == std::string::npos