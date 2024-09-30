#include "pch.h"
#include "soundsystem.h"
#include "features/mgr.h"

extern void ShowDonationWindow();

std::vector<std::string> donators = {
    "berrymuffin",
    "Dustin Eastwood",
    "Dwolf98",
    "KaiQ",
    "MC Silver",
    "Osama aj",
    "Pol3 Million",
    "Seemann",
    "spdfnpe"
};

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved) {
    if (nReason == DLL_PROCESS_ATTACH) {
        if (gConfig.ReadBoolean("MISC", "ShowDonationPopup", true)) {
            ShowDonationWindow();
            gConfig.WriteBoolean("MISC", "ShowDonationPopup", false);
        }

        plugin::Events::gameProcessEvent += [](){
            SoundSystem.Process();
        };

        plugin::Events::shutdownRwEvent += [](){
            SoundSystem.Clear();
        };

        Events::initGameEvent += []() {
            bool ImVehFtInstalled = GetModuleHandle("ImVehFt.asi");
            bool ImVehFtFixInstalled = GetModuleHandle("ImVehFtFix.asi");
            bool AVSInstalled = GetModuleHandle("AdvancedVehicleSirens.asi");
            bool EarShot = GetModuleHandle("EarShot.asi");

            gLogger->flush_on(spdlog::level::info);
            gLogger->set_pattern("%v"); 
            gLogger->info("Starting " MOD_TITLE " (" __DATE__ ")\nAuthor: Grinch_\nDiscord: "
                                        DISCORD_INVITE "\nPatreon: " PATREON_LINK "\nMore Info: " GITHUB_LINK "");

            // date time
            SYSTEMTIME st;
            GetSystemTime(&st);
            gLogger->info("Date: {}-{}-{} Time: {}:{}\n", st.wYear, st.wMonth, st.wDay,
                                        st.wHour, st.wMinute);
            gLogger->info("\nDonators:");
            for (const auto& name : donators) {
                gLogger->info("- {}", name);
            }
            
            gLogger->set_pattern("[%L] %v");
            SoundSystem.Init();

            /*
                Had to put this in place since some people put the folder in root
                directory and the asi in modloader. Why??
            */
            if (!std::filesystem::is_directory(PLUGIN_PATH((char*)MOD_NAME))) {
                std::string msg = std::format("{} folder not found. You need to put both '{}.asi' & '{}' folder in the same directory", MOD_NAME, MOD_NAME, MOD_NAME);
                gLogger->error(msg.c_str());
                MessageBox(NULL, msg.c_str(), MOD_NAME, MB_ICONERROR);
                return;
            }

            if (gConfig.ReadBoolean("MISC", "ShowDeprecationMessage", true) 
            && (ImVehFtInstalled || ImVehFtFixInstalled || AVSInstalled || EarShot)) {
                std::string str = "ModelExtras contain the functions of these plugins,\n\n";
                
                if (ImVehFtInstalled) str += "- ImVehFt.asi\n";
                if (ImVehFtFixInstalled) str += "- ImVehFtFix.asi\n";
                if (AVSInstalled) str += "- AdvancedVehicleSirens.asi\n";
                if (EarShot) str += "- EarShot.asi\n";

                str += "\nIt is recommanded to remove them to ensure proper gameplay.";
                MessageBox(RsGlobal.ps->window, str.c_str(), "Deprecated plugins found!", MB_OK);
                gConfig.WriteBoolean("MISC", "ShowDeprecationMessage", false);
            }

            FeatureMgr::Initialize();
        };
    }
    return TRUE;
}