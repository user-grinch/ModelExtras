#include "pch.h"
#include "defines.h"
#include "loader.h"

constexpr std::string_view SeverityToString(AixLog::Severity severity) noexcept
{
    switch (severity)
    {
        case AixLog::Severity::trace:   return "TRACE";
        case AixLog::Severity::debug:   return "DEBUG";
        case AixLog::Severity::info:    return "INFO ";
        case AixLog::Severity::notice:  return "NOTE ";
        case AixLog::Severity::warning: return "WARN ";
        case AixLog::Severity::error:   return "ERROR";
        case AixLog::Severity::fatal:   return "FATAL";
        default:                        return "INFO ";
    }
}

inline std::string FormatLogLine(const AixLog::Metadata &metadata, const std::string &message)
{
    std::string timeStr = metadata.timestamp ? metadata.timestamp.to_string("%H:%M:%S.#ms") : "00:00:00.000";
    return std::format("[{}] [{}] {}\n", timeStr, SeverityToString(metadata.severity), message);
}

struct ModLogSink : public AixLog::Sink
{
    ModLogSink(const AixLog::Filter &filter, const std::string &filename)
        : AixLog::Sink(filter)
    {
        ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
        SYSTEMTIME st;
        GetLocalTime(&st);
        std::string dateBuf = std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
                                          st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        ofs << "//==========================================================\n"
            << "// " << MOD_TITLE << " (Built: " << __DATE__ << ")\n"
            << "// Authors:    Grinch_, Caner Karaca, Ameer, KaiQ\n"
            << "// GitHub:     " << GITHUB_LINK << "\n"
            << "// Discord:    " << DISCORD_INVITE << "\n"
            << "// Started at: " << dateBuf << "\n"
            << "//==========================================================\n\n";
        ofs.flush();
    }

    ~ModLogSink() override
    {
        ofs.close();
    }

    void log(const AixLog::Metadata &metadata, const std::string &message) override
    {
        if (metadata.severity < AixLog::Severity::info && !gVerboseLogging)
        {
            return;
        }

        ofs << FormatLogLine(metadata, message);
        ofs.flush();
    }

protected:
    mutable std::ofstream ofs;
};

struct ModConsoleSink : public AixLog::Sink
{
    ModConsoleSink(const AixLog::Filter &filter) : AixLog::Sink(filter) {}

    void log(const AixLog::Metadata &metadata, const std::string &message) override
    {
        if (metadata.severity < AixLog::Severity::info && !gVerboseLogging)
        {
            return;
        }

        std::cout << FormatLogLine(metadata, message);
    }
};

void InitLog()
{
    if (!gConfig.ReadBoolean("CONFIG", "EnableLogging", true))
    {
        AixLog::Log::init({});
        return;
    }

    auto sink_cout = std::make_shared<ModConsoleSink>(AixLog::Severity::trace);
    auto sink_file = std::make_shared<ModLogSink>(AixLog::Severity::trace, std::string(MOD_NAME) + ".log");
    AixLog::Log::init({sink_cout, sink_file});

    LOG(INFO) << "[Core] Initializing " << MOD_TITLE << "...";
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
                LOG(INFO) << "[Core] Verbose logging is disabled (enable 'VerboseLogging' in ModelExtras.ini for debug output).";
            }

            HMODULE hCleo = GetModuleHandleA("CLEO.asi");
            if (!hCleo)
            {
                MessageBoxA(RsGlobal.ps ? RsGlobal.ps->window : NULL,
                            "CLEO Library 4.4.4 or above is required!\nCLEO.asi was not found.",
                            "ModelExtras", MB_OK | MB_ICONERROR);
                LOG(ERROR) << "[CLEO] CLEO Library 4.4.4 or above is required! CLEO.asi was not found.";
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
                        std::string msg = std::format(
                            "CLEO Library 4.4.4 or above is required!\n"
                            "Detected CLEO version: {}.{}.{}\n"
                            "Please update CLEO to 4.4.4 or above to avoid audio issues.",
                            major, minor, patch);

                        MessageBoxA(RsGlobal.ps ? RsGlobal.ps->window : NULL, msg.c_str(), "ModelExtras", MB_OK | MB_ICONWARNING);
                        LOG(WARNING) << "[CLEO] " << msg;
                    }
                    else
                    {
                        LOG(INFO) << "[CLEO] CLEO version " << major << "." << minor << "." << patch << " detected.";
                    }
                }
            }
        };
        ModelExtras::Init();
    }
    return TRUE;
}
