#include "pch.h"
#include "features/mgr.h"

extern void ShowDonationWindow();

std::vector<std::string> donators = {
    "Agha",
    "berrymuffin",
    "Damian Jurkiewicz",
    "Dustin Eastwood",
    "Dwolf98",
    "Francisco Flores",
    "KaiQ",
    "MC Silver",
    "Osama aj",
    "Pol3 Million",
    "Seemann",
    "spdfnpe"};

void InitLogFile()
{
    static bool flag = true;
    if (!flag)
    {
        return;
    }
    gLogger->flush_on(spdlog::level::debug);
    spdlog::set_level(spdlog::level::debug);
    gLogger->set_pattern("%v");
    gLogger->info("Starting " MOD_TITLE " (" __DATE__ ")\nAuthor: Grinch_\nDiscord: " DISCORD_INVITE "\nPatreon: " PATREON_LINK "\nMore Info: " GITHUB_LINK "");

    // date time
    SYSTEMTIME st;
    GetSystemTime(&st);
    gLogger->info("Date: {}-{}-{} Time: {}:{}\n", st.wYear, st.wMonth, st.wDay,
                  st.wHour, st.wMinute);
    gLogger->info("\nDonators:");
    for (const auto &name : donators)
    {
        gLogger->info("- {}", name);
    }

    gLogger->set_pattern("[%L] %v");
    flag = false;
}

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    if (nReason == DLL_PROCESS_ATTACH)
    {
        if (gConfig.ReadBoolean("MISC", "ShowDonationPopup", true))
        {
            ShowDonationWindow();
            gConfig.WriteBoolean("MISC", "ShowDonationPopup", false);
        }

        gVerboseLogging = gConfig.ReadBoolean("CONFIG", "VerboseLogging", false);

        Events::initRwEvent.after += []()
        {
            bool fxFuncs = GetModuleHandle("FxsFuncs.asi");
            bool ola = GetModuleHandle("III.VC.SA.LimitAdjuster.asi");
            bool fla = GetModuleHandle("$fastman92limitAdjuster.asi");
            bool gtweaker = GetModuleHandle("GraphicsTweaker.SA.asi");
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
                MessageBox(RsGlobal.ps->window, "CLEO Library 5.0 or above required!", "ModelExtras", MB_OK);
                gLogger->error("CLEO Library 5.0 or above required!");
            }
        };

        Events::initGameEvent += []()
        {
            bool ImVehFtInstalled = GetModuleHandle("ImVehFt.asi");
            bool ImVehFtFixInstalled = GetModuleHandle("ImVehFtFix.asi");
            bool AVSInstalled = GetModuleHandle("AdvancedVehicleSirens.asi");
            bool EarShot = GetModuleHandle("EarShot.asi");
            bool PedFuncs = GetModuleHandle("PedFuncs.asi");

            InitLogFile();

            /*
                Had to put this in place since some people put the folder in root
                directory and the asi in modloader. Why??
            */
            if (!std::filesystem::is_directory(PLUGIN_PATH((char *)MOD_NAME)))
            {
                std::string msg = std::format("{} folder not found. You need to put both '{}.asi' & '{}' folder in the same directory", MOD_NAME, MOD_NAME, MOD_NAME);
                gLogger->error(msg.c_str());
                MessageBox(RsGlobal.ps->window, msg.c_str(), MOD_NAME, MB_ICONERROR);
                return;
            }

            if (gConfig.ReadBoolean("MISC", "ShowIncompatibleWarning", true) && (ImVehFtInstalled || ImVehFtFixInstalled || AVSInstalled || EarShot))
            {
                std::string str = "ModelExtras contain the functions of these plugins,\n\n";

                if (ImVehFtInstalled)
                    str += "- ImVehFt.asi\n";
                if (ImVehFtFixInstalled)
                    str += "- ImVehFtFix.asi\n";
                if (AVSInstalled)
                    str += "- AdvancedVehicleSirens.asi\n";
                if (EarShot)
                    str += "- EarShot.asi\n";
                if (PedFuncs)
                    str += "- PedFuncs.asi\n";

                str += "\nIt is recommanded to remove them to ensure proper gameplay.";
                MessageBox(RsGlobal.ps->window, str.c_str(), "Incompatible plugins found!", MB_OK);
            }

            if (gConfig.ReadBoolean("MISC", "ShowGraphicsTweakerWarning", true))
            {
                std::string str = "Using GraphicsTweaker may result in visual anomalies.\n\n";
                str += "Set these values to following to avoid issues,\n";
                str += "1. MultAmbientNight = 1.0\n2. MultColorFilterNight = 1.0\n";
                MessageBox(RsGlobal.ps->window, str.c_str(), "GraphicsTweaker Found!", MB_OK);
                gConfig.WriteBoolean("MISC", "ShowGraphicsTweakerWarning", false);
            }
        };
        FeatureMgr::Initialize();
    }
    return TRUE;
}