#pragma once
#include <plugin.h>
#define PATRON_BUILD 1

#define MOD_DATA_PATH(x) PLUGIN_PATH((char *)("ModelExtras/" x))

#define MOD_NAME "ModelExtras"
#define MOD_VERSION "2.1"
#define MOD_VERSION_NUMBER 21000
#define MOD_VERSION_SUFFIX "-beta"

#ifdef PATRON_BUILD
#define MOD_TITLE MOD_NAME " v" MOD_VERSION MOD_VERSION_SUFFIX " (Patron Build)"
#else
#define MOD_TITLE MOD_NAME " v" MOD_VERSION MOD_VERSION_SUFFIX
#endif

#define DISCORD_INVITE "https://discord.gg/AduJVdyqCD"
#define GITHUB_LINK "https://github.com/user-grinch/ModelExtras"
#define PATREON_LINK "https://www.patreon.com/grinch_"

#define STR_FOUND(x, y) x.find(y) != std::string::npos