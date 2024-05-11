#include "pch.h"
#include "features/vehicle/plate.h"
#include "features/vehicle/handlebar.h"
#include "features/vehicle/wheelhub.h"
#include "features/vehicle/steerwheel.h"
#include "features/vehicle/lights.h"
#include "features/vehicle/indicators.h"
#include "features/vehicle/spotlights.h"
#include "features/vehicle/sirens.h"
#include "features/weapon/bodystate.h"
#include "features/weapon/bloodremap.h"
#include "features/common/randomizer.h"
#include "features/common/remap.h"
#include "soundsystem.h"
#include "features/mgr.h"

static ThiscallEvent <AddressList<0x5343B2, H_CALL>, PRIORITY_BEFORE, ArgPickN<CObject*, 0>, void(CObject*)> objectRenderEvent;

static void InitFeatures() {
    Remap.Initialize();
    Randomizer.Initialize();
    Indicator.Initialize();
    Lights.Initialize();
    VehicleSirens.Initialize();

    plugin::Events::vehicleRenderEvent.before += [](CVehicle* vehicle) {
        VehicleMaterials::RestoreMaterials();
        VehicleMaterials::OnRender(vehicle);
    };
    plugin::Events::vehicleSetModelEvent += VehicleMaterials::OnModelSet;
}

static void ProcessNodesRecursive(RwFrame * frame, void* pEntity, eModelEntityType type) {
    if(frame) {
        const std::string name = GetFrameNodeName(frame);

        if (type == eModelEntityType::Vehicle) {
            CVehicle *pVeh = static_cast<CVehicle*>(pEntity);
            if (gConfig.ReadBoolean("FEATURES", "RotateHandleBars", false)) {
                HandleBar.Process(frame, pVeh);
            }
            if (gConfig.ReadBoolean("FEATURES", "RotateSteerWheel", false)) {
                SteerWheel.Process(frame, pVeh);
            }
            WheelHub.Process(frame, pVeh);
        }

        CVehicle *pVeh = static_cast<CVehicle*>(pEntity);
        if ((name[0] == 'x' && name[1] == '_') || (name[0] == 'f' && name[1] == 'c' && name[2] == '_')){
            if (type == eModelEntityType::Vehicle) {
                Randomizer.Process(frame, static_cast<void*>(pVeh), type);
            } else if (type == eModelEntityType::Weapon) {
                CWeapon *pWep = static_cast<CWeapon*>(pEntity);
                BodyState.Process(frame, pWep);
                BodyState.ProcessZen(frame, pWep);
                BloodRemap.Process(frame, pWep);
                Randomizer.Process(frame, static_cast<void*>(pWep), type);
            } else if (type == eModelEntityType::Object) {

                /*
                    processing weapon & jetpack pickups here
                */
                CWeapon *pWep = static_cast<CWeapon*>(pEntity);
                BodyState.Process(frame, pWep);
                BodyState.ProcessZen(frame, pWep);
            } else if (type == eModelEntityType::Ped) {
                Randomizer.Process(frame, pEntity, type);
            }
        }
 

        // Compatibility with older plugins
        if (name == "spotlight_dummy") {
            SpotLight.Process(frame, pVeh);
        } 

        // LicensePlate.Process(frame, pVeh);

        if (RwFrame * newFrame = frame->child) {
            ProcessNodesRecursive(newFrame, pEntity, type);
        }
        if (RwFrame * newFrame = frame->next) {
            ProcessNodesRecursive(newFrame, pEntity, type);
        }
    }
    return;
}
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
            // InitFeatures();

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
            FeatureMgr.Initialize(static_cast<CEntity*>(pVeh), (RwFrame *)pVeh->m_pRwClump->object.parent);
        };

        Events::vehicleRenderEvent.before += [](CVehicle *pVeh) {
            FeatureMgr.Process(static_cast<CEntity*>(pVeh));
        };

        // Events::vehicleRenderEvent += [](CVehicle* pVeh) {
        //     ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh, eModelEntityType::Vehicle);
        // };

        // objectRenderEvent += [](CObject *pObj) {
        //     ProcessNodesRecursive((RwFrame *)pObj->m_pRwClump->object.parent, pObj, eModelEntityType::Object);
        // };

        // Events::pedRenderEvent += [](CPed* pPed) {
        //     // peds
        //     ProcessNodesRecursive((RwFrame *)pPed->m_pRwClump->object.parent, pPed, eModelEntityType::Ped);

        //     // jetpack
        //     CTaskSimpleJetPack *pTask = pPed->m_pIntelligence->GetTaskJetPack();
        //     if (pTask && pTask->m_pJetPackClump) {
        //         ProcessNodesRecursive((RwFrame *)pTask->m_pJetPackClump->object.parent, pPed, eModelEntityType::Weapon);
        //     }

        //     // weapons
        //     CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        //     if (pWeapon) {
        //         eWeaponType weaponType = pWeapon->m_eWeaponType;
        //         CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
        //         if (pWeaponInfo) {
        //             CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
        //             if (pWeaponModelInfo && pWeaponModelInfo->m_pRwClump) {
        //                 ProcessNodesRecursive((RwFrame *)pWeaponModelInfo->m_pRwClump->object.parent, pWeapon, eModelEntityType::Weapon);
        //             }
        //         }
        //     }
        // };
    }
    return TRUE;
}

