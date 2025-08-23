#include "pch.h"
#include "defines.h"

std::shared_ptr<spdlog::logger> gLogger = spdlog::basic_logger_mt("Logger", MOD_NAME ".log", true);
CIniReader gConfig(MOD_NAME ".ini");
bool gVerboseLogging;
unsigned int FramePluginOffset;