#include "pch.h"
#include "defines.h"
#include "features/mgr.h"

#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>

#define GET_SCRIPT_STRUCT_NAMED 0xAAA

extern void ShowDonationWindow();
extern void TrainerInit();

std::vector<std::string> donators = {
    "Agha",
    "berrymuffin",
    "blackOS"
    "Boris Ilincic",
    "Damix",
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
        if (gConfig.ReadBoolean("CONFIG", "ShowDonationPopup", true))
        {
            ShowDonationWindow();
            gConfig.WriteBoolean("CONFIG", "ShowDonationPopup", false);
        }

        gVerboseLogging = gConfig.ReadBoolean("CONFIG", "VerboseLogging", false);
        if (!gVerboseLogging) {
            gLogger->info("Enable 'VerboseLogging' in ModelExtras.ini to display model-related errors.");
        }

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
                gLogger->error("CLEO Library 4.4 or above is required!");
            }
        };

        Events::initGameEvent += []()
        {
            bool CLEOInstalled = GetModuleHandle("CLEO.asi");
            bool ImVehFtInstalled = GetModuleHandle("ImVehFt.asi");
            bool ImVehFtFixInstalled = GetModuleHandle("ImVehFtFix.asi");
            bool AVSInstalled = GetModuleHandle("AdvancedVehicleSirens.asi");
            bool EarShot = GetModuleHandle("EarShot.asi");
            bool gtweaker = GetModuleHandle("GraphicsTweaker.SA.asi");
            bool PedFuncs = GetModuleHandle("PedFuncs.asi");
            bool GrinchTrainer = GetModuleHandle("GrinchTrainerSA.asi");
            bool BackFireZAZInstalled = false;
            bool BackFireJDRInstalled = false;

            if (CLEOInstalled)
            {
                int script = NULL;
                plugin::Command<GET_SCRIPT_STRUCT_NAMED>("IFLAME", &script);
                BackFireZAZInstalled = script != NULL;
                plugin::Command<GET_SCRIPT_STRUCT_NAMED>("Backfir", &script);
                BackFireJDRInstalled = script != NULL;
            }

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

            if (gConfig.ReadBoolean("CONFIG", "ShowIncompatibleWarning", true) && (BackFireJDRInstalled || BackFireZAZInstalled || ImVehFtInstalled || ImVehFtFixInstalled || AVSInstalled || EarShot))
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
                if (BackFireZAZInstalled)
                    str += "- Back-fire.cs by ZAZ\n";
                if (BackFireJDRInstalled)
                    str += "- Backfire - ALS.cs by Junior-Djjr\n";

                str += "\nRemove them to continue playing the game.";
                MessageBox(RsGlobal.ps->window, str.c_str(), "Incompatible plugins found!", MB_OK);
                gLogger->error(str);
                exit(EXIT_FAILURE);
            }

            if (gtweaker && gConfig.ReadBoolean("CONFIG", "ShowGraphicsTweakerWarning", true))
            {
                std::string str = "Using GraphicsTweaker may result in visual anomalies.\n\n";
                str += "Set these values to following to avoid issues,\n";
                str += "1. MultAmbientNight = 1.0\n2. MultColorFilterNight = 1.0\n";
                str += "\n\nSet ShowGraphicsTweakerWarning=False in ModelExtras.ini to remove this popup.\n";
                MessageBox(RsGlobal.ps->window, str.c_str(), "GraphicsTweaker Found!", MB_OK);
                gLogger->error(str);
            }

            if (GrinchTrainer && gConfig.ReadBoolean("CONFIG", "DeveloperMode", false))
            {
                gLogger->info("GrinchTrainerSA found. Registering...");
                Events::processScriptsEvent += []()
                {
                    TrainerInit();
                };
            }
        };

        FeatureMgr::Initialize();
    }
    return TRUE;
}