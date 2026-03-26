#include "pch.h"
#include "defines.h"
#include "features/mgr.h"

extern void ShowDonationWindow();
extern void InjectImGuiHooks();

std::vector<std::string> donators = {
    "Wei Woo",
    "©Wishy",
    "Alexander Alexander",
    "Imad Fikri",
    "ID_not4ound",
    "Macc",
    "lemaze93",
    "XG417",
    "Ruethy",
    "Flaqko _GTA",
    "MG45",
    "Boris Ilincic",
    "Damix",
    "spdfnpe",
    "Pol3 Million",
    "Bubby Jackson",
    "Keith Ferrell",
    "Clayton Morrison",
    "SimBoRRis",
    "Agha"
};

void InitLogFile()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

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

    if (!donators.empty())
    {
        header += "Donators:\n";
        for (const auto& name : donators)
        {
            header += "- " + name + "\n";
        }
    }
    LOG(INFO) << header;
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
#if !PATRON_BUILD
        if (gConfig.ReadBoolean("CONFIG", "ShowDonationPopup", true))
        {
            ShowDonationWindow();
            gConfig.WriteBoolean("CONFIG", "ShowDonationPopup", false);
        }
#endif
        if (gConfig.ReadBoolean("CONFIG", "DeveloperMode", false))
        {
            InjectImGuiHooks();
            LOG(INFO) << "DeveloperMode enabled, injecting ImGui hooks...";
        }

        gVerboseLogging = gConfig.ReadBoolean("CONFIG", "VerboseLogging", false);

        Events::initScriptsEvent.after += []()
        {
            if (!gVerboseLogging)
            {
                LOG(INFO) << "Enable 'VerboseLogging' in ModelExtras.ini to display model-related errors.";
            }
        };

        Events::initRwEvent.after += []()
        {
            bool fxFuncs = GetModuleHandle("FxsFuncs.asi");
            bool ola = GetModuleHandle("III.VC.SA.LimitAdjuster.asi");
            bool fla = GetModuleHandle("$fastman92limitAdjuster.asi");
            bool cleo = GetModuleHandle("CLEO.asi");

            if ((fxFuncs && !(ola || fla)))
            {
                std::string str = "Install any of the below,\n\n";

                str += "- Open Limit Adjuster (Recommanded)\n";
                str += "- Fastman92 Limit Adjuster (Increase IDE limits)\n";
                MessageBox(RsGlobal.ps->window, str.c_str(), "LimitAdjuster required!", MB_OK);
            }

            if (!cleo)
            {
                MessageBox(RsGlobal.ps->window, "CLEO Library 4.4 or above is required!", "ModelExtras", MB_OK);
                LOG(ERROR) << "CLEO Library 4.4 or above is required!";
            }
        };

        Events::initGameEvent += []()
        {
            bool ImVehFtInstalled = GetModuleHandle("ImVehFt.asi");
            bool ImVehFtFixInstalled = GetModuleHandle("ImVehFtFix.asi");
            bool AVSInstalled = GetModuleHandle("AdvancedVehicleSirens.asi");
            bool PedFuncs = GetModuleHandle("PedFuncs.asi");

            InitLogFile();

            if (gConfig.ReadBoolean("CONFIG", "ShowIncompatibleWarning", true) && (ImVehFtInstalled || ImVehFtFixInstalled || AVSInstalled))
            {
                std::string str = "ModelExtras contain the functions of these plugins,\n\n";

                if (ImVehFtInstalled)
                    str += "- ImVehFt.asi\n";
                if (ImVehFtFixInstalled)
                    str += "- ImVehFtFix.asi\n";
                if (AVSInstalled)
                    str += "- AdvancedVehicleSirens.asi\n";
                if (PedFuncs)
                    str += "- PedFuncs.asi\n";

                str += "\nRemove them to continue playing the game.";
                MessageBox(RsGlobal.ps->window, str.c_str(), "Incompatible plugins found!", MB_OK);
                LOG(ERROR) << str;
                exit(EXIT_FAILURE);
            }
            return TRUE;
        };
        FeatureMgr::Initialize();
    }
    return TRUE;
}