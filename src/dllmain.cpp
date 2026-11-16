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
        };
        ModelExtras::Init();
    }
    return TRUE;
}
