#include "pch.h"
#include "defines.h"
#include "loader.h"

void InitLog()
{
    if (!gConfig.ReadBoolean("CONFIG", "EnableLogging", true))
    {
        AixLog::Log::init({});
        return;
    }

    auto sink_cout = std::make_shared<AixLog::SinkCout>(AixLog::Severity::debug);
    auto sink_file = std::make_shared<AixLog::SinkFile>(AixLog::Severity::debug, std::string(MOD_NAME) + ".log");
    AixLog::Log::init({sink_cout, sink_file});

    std::string header = "Starting " + std::string(MOD_TITLE) + " (" + __DATE__ + ")\n"
                         "Authors: Grinch_, Caner Karaca, Ameer\n"
                         "Discord: " + DISCORD_INVITE + "\n"
                         "More Info: " + GITHUB_LINK + "\n";
    
    SYSTEMTIME st;
    GetSystemTime(&st);
    char timeBuf[64];
    sprintf_s(timeBuf, "Date: %04d-%02d-%02d Time: %02d:%02d\n", 
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    
    header += timeBuf;
    LOG(INFO) << header;
}

#pragma comment(lib, "version.lib")

static bool GetCleoVersion(HMODULE hCleo, int &major, int &minor, int &patch)
{
    major = minor = patch = 0;

    // Method 1: Exported function (CLEO 5+)
    typedef DWORD(WINAPI *tCLEO_GetVersion)();
    auto pfnGetVersion = (tCLEO_GetVersion)GetProcAddress(hCleo, "CLEO_GetVersion");
    if (pfnGetVersion)
    {
        DWORD ver = pfnGetVersion();
        major = (ver >> 24) & 0xFF;
        minor = (ver >> 16) & 0xFF;
        patch = (ver >> 8) & 0xFF;
        if (major > 0)
        {
            return true;
        }
    }

    // Method 2: PE File Version Info from CLEO.asi (works for all CLEO 4 and 5 versions)
    char szModulePath[MAX_PATH] = {0};
    if (GetModuleFileNameA(hCleo, szModulePath, MAX_PATH) > 0)
    {
        DWORD dwHandle = 0;
        DWORD dwSize = GetFileVersionInfoSizeA(szModulePath, &dwHandle);
        if (dwSize > 0)
        {
            std::vector<BYTE> data(dwSize);
            if (GetFileVersionInfoA(szModulePath, dwHandle, dwSize, data.data()))
            {
                VS_FIXEDFILEINFO *pFileInfo = nullptr;
                UINT uLen = 0;
                if (VerQueryValueA(data.data(), "\\", (LPVOID *)&pFileInfo, &uLen) && pFileInfo && uLen >= sizeof(VS_FIXEDFILEINFO))
                {
                    major = HIWORD(pFileInfo->dwFileVersionMS);
                    minor = LOWORD(pFileInfo->dwFileVersionMS);
                    patch = HIWORD(pFileInfo->dwFileVersionLS);
                    return true;
                }
            }
        }
    }
    return false;
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

            HMODULE hCleo = GetModuleHandleA("CLEO.asi");
            if (!hCleo)
            {
                MessageBoxA(RsGlobal.ps ? RsGlobal.ps->window : NULL,
                            "CLEO Library 4.4.4 or above is required!\nCLEO.asi was not found.",
                            "ModelExtras", MB_OK | MB_ICONERROR);
                LOG(ERROR) << "CLEO Library 4.4.4 or above is required! CLEO.asi was not found.";
            }
            else
            {
                int major = 0, minor = 0, patch = 0;
                if (GetCleoVersion(hCleo, major, minor, patch))
                {
                    // Minimum version required: CLEO 4.4.4 (or CLEO 5+)
                    bool isOutdated = (major < 4) ||
                                      (major == 4 && (minor < 4 || (minor == 4 && patch < 4)));

                    if (isOutdated)
                    {
                        char msg[256];
                        sprintf_s(msg,
                                  "CLEO Library 4.4.4 or above is required!\n"
                                  "Detected CLEO version: %d.%d.%d\n"
                                  "Please update CLEO to 4.4.4 or above to avoid audio issues.",
                                  major, minor, patch);

                        MessageBoxA(RsGlobal.ps ? RsGlobal.ps->window : NULL, msg, "ModelExtras", MB_OK | MB_ICONWARNING);
                        LOG(WARNING) << msg;
                    }
                    else
                    {
                        LOG(INFO) << "CLEO version " << major << "." << minor << "." << patch << " detected.";
                    }
                }
            }
        };
        ModelExtras::Init();
    }
    return TRUE;
}