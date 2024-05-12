#include "pch.h"
#include "features/common/remap.h"
#include "soundsystem.h"
#include "features/mgr.h"

static ThiscallEvent <AddressList<0x5343B2, H_CALL>, PRIORITY_BEFORE, ArgPickN<CObject*, 0>, void(CObject*)> objectRenderEvent;

extern void ShowDonationWindow();

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved) {
    if (nReason == DLL_PROCESS_ATTACH) {

        if (gConfig.ReadBoolean("MISC", "ShowDonationPopup", true)) {
            ShowDonationWindow();
            gConfig.WriteBoolean("MISC", "ShowDonationPopup", false);
        }

        Events::initGameEvent += []() {
            bool ImVehFtInstalled = GetModuleHandle("ImVehFt.asi");
            bool ImVehFtFixInstalled = GetModuleHandle("ImVehFtFix.asi");
            bool AVSInstalled = GetModuleHandle("AdvancedVehicleSirens.asi");

            gLogger->flush_on(spdlog::level::info);
            gLogger->set_pattern("%v"); 
            gLogger->info("Starting " MOD_TITLE " (" __DATE__ ")\nAuthor: Grinch_\nDiscord: "
                                        DISCORD_INVITE "\nPatreon: " PATREON_LINK "\nMore Info: " GITHUB_LINK "");

            // date time
            SYSTEMTIME st;
            GetSystemTime(&st);
            gLogger->info("Date: {}-{}-{} Time: {}:{}\n", st.wYear, st.wMonth, st.wDay,
                                        st.wHour, st.wMinute);
            gLogger->set_pattern("[%L] %v");
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
            SoundSystem.Inject();
            SoundSystem.Init(RsGlobal.ps->window);
            InitRandom();

            if (gConfig.ReadBoolean("MISC", "ShowDeprecationMessage", true) 
            && (ImVehFtInstalled || ImVehFtFixInstalled || AVSInstalled)) {
                std::string str = "ModelExtras contain the functions of these plugins,\n\n";
                
                if (ImVehFtInstalled) str += "- ImVehFt.asi\n";
                if (ImVehFtFixInstalled) str += "- ImVehFtFix.asi\n";
                if (AVSInstalled) str += "- AdvancedVehicleSirens.asi\n";

                str += "\nIt is recommanded to remove them to ensure proper gameplay.";
                MessageBox(RsGlobal.ps->window, str.c_str(), "Deprecated plugins found!", MB_OK);
                gConfig.WriteBoolean("MISC", "ShowDeprecationMessage", false);
            }
        };

        Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model) {
            FeatureMgr.Initialize(static_cast<void*>(pVeh), (RwFrame *)pVeh->m_pRwClump->object.parent, eModelEntityType::Vehicle);
        };

        Events::vehicleRenderEvent.before += [](CVehicle *pVeh) {
            FeatureMgr.Process(static_cast<void*>(pVeh), eModelEntityType::Vehicle);
        };

        Events::pedRenderEvent += [](CPed* pPed) {
            FeatureMgr.Initialize(static_cast<void*>(pPed), 
                (RwFrame *)pPed->m_pRwClump->object.parent, eModelEntityType::Ped);
            FeatureMgr.Process(static_cast<void*>(pPed), eModelEntityType::Ped);

            // jetpack
            CTaskSimpleJetPack *pTask = pPed->m_pIntelligence->GetTaskJetPack();
            if (pTask && pTask->m_pJetPackClump) {
                FeatureMgr.Initialize(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), 
                    (RwFrame *)pTask->m_pJetPackClump->object.parent, eModelEntityType::Weapon);
                FeatureMgr.Process(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), eModelEntityType::Weapon);
            }

            // weapons
            CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
            if (pWeapon) {
                eWeaponType weaponType = pWeapon->m_eWeaponType;
                CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
                if (pWeaponInfo) {
                    CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
                    if (pWeaponModelInfo && pWeaponModelInfo->m_pRwClump) {
                        FeatureMgr.Initialize(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), 
                            (RwFrame *)pWeaponModelInfo->m_pRwClump->object.parent, eModelEntityType::Weapon);
                        FeatureMgr.Process(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), eModelEntityType::Weapon);
                    }
                }
            }
        };

        objectRenderEvent += [](CObject *pObj) {
            FeatureMgr.Initialize(static_cast<void*>(pObj), 
                (RwFrame *)pObj->m_pRwClump->object.parent, eModelEntityType::Object);
            FeatureMgr.Process(static_cast<void*>(pObj), eModelEntityType::Object);
        };
    }
    return TRUE;
}

