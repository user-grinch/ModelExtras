#include "pch.h"
#include "defines.h"
#include "loader.h"

void InitLog()
{
    auto sink_cout = std::make_shared<AixLog::SinkCout>(AixLog::Severity::debug);
    auto sink_file = std::make_shared<AixLog::SinkFile>(AixLog::Severity::debug, std::string(MOD_NAME) + ".log");
    AixLog::Log::init({sink_cout, sink_file});

    std::string header = "Starting " + std::string(MOD_TITLE) + " (" + __DATE__ + ")\n"
                         "Author: Grinch_\n"
                         "Discord: " + DISCORD_INVITE + "\n"
                         "Patreon: " + PATREON_LINK + "\n"
                         "More Info: " + GITHUB_LINK + "\n";
    
    SYSTEMTIME st;
    GetSystemTime(&st);
    char timeBuf[64];
    sprintf_s(timeBuf, "Date: %04d-%02d-%02d Time: %02d:%02d\n", 
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    
    header += timeBuf;
    LOG(INFO) << header;
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        gVerboseLogging = gConfig.ReadBoolean("CONFIG", "VerboseLogging", false);

        Events::initGameEvent += []()
        {
            InitLog();
            if (!gVerboseLogging)
            {
                LOG(INFO) << "Enable 'VerboseLogging' in ModelExtras.ini to display model-related errors.";
            }

            if (!GetModuleHandle("CLEO.asi"))
            {
                MessageBox(RsGlobal.ps->window, "CLEO Library 4.4 or above is required!", "ModelExtras", MB_OK);
                LOG(ERROR) << "CLEO Library 4.4 or above is required!";
            }
        };
        ModelExtras::Init();
    }
    return TRUE;
}