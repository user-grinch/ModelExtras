#include "pch.h"
#include "features/vehicle/brakes.h"
#include "features/vehicle/chain.h"
#include "features/vehicle/meter.h"
#include "features/vehicle/gear.h"
#include "features/vehicle/plate.h"
#include "features/vehicle/handlebar.h"
#include "features/vehicle/turnlights.h"
#include "features/vehicle/lights.h"
#include "features/vehicle/indicators.h"
#include "features/vehicle/sirens.h"
#include "features/weapon/bodystate.h"
#include "features/weapon/bloodremap.h"
#include "features/common/randomizer.h"
#include "features/common/remap.h"
#include "soundsystem.h"


static ThiscallEvent <AddressList<0x5343B2, H_CALL>, PRIORITY_BEFORE, ArgPickN<CObject*, 0>, void(CObject*)> objectRenderEvent;

static void InitFeatures() {
    Remap.Initialize();
    Randomizer.Initialize();
    // TurnLights.Initialize();
    VehicleLights::RegisterEvents();
    VehicleIndicators::RegisterEvents();
    VehicleSirens::RegisterEvents();

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
            HandleBar.Process(frame, pVeh);
        }

        if ((name[0] == 'x' && name[1] == '_')|| (name[0] == 'f' && name[1] == 'c' && name[2] == '_')) {
            if (type == eModelEntityType::Vehicle) {
                CVehicle *pVeh = static_cast<CVehicle*>(pEntity);
                Chain.Process(frame, pVeh);
                FrontBrake.Process(frame, pVeh);
                RearBrake.Process(frame, pVeh);
                GearMeter.Process(frame, pVeh);
                OdoMeter.Process(frame, pVeh);
                RpmMeter.Process(frame, pVeh);
                SpeedMeter.Process(frame, pVeh);
                Clutch.Process(frame, pVeh);
                GearLever.Process(frame, pVeh);
                GearSound.Process(frame, pVeh);
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

BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved) {
    if (nReason == DLL_PROCESS_ATTACH) {
        
        Events::initGameEvent += []() {
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
            InitFeatures();
        };

        Events::vehicleRenderEvent += [](CVehicle* pVeh) {
            ProcessNodesRecursive((RwFrame *)pVeh->m_pRwClump->object.parent, pVeh, eModelEntityType::Vehicle);
        };

        objectRenderEvent += [](CObject *pObj) {
            ProcessNodesRecursive((RwFrame *)pObj->m_pRwClump->object.parent, pObj, eModelEntityType::Object);
        };

        Events::pedRenderEvent += [](CPed* pPed) {
            // peds
            ProcessNodesRecursive((RwFrame *)pPed->m_pRwClump->object.parent, pPed, eModelEntityType::Ped);

            // jetpack
            CTaskSimpleJetPack *pTask = pPed->m_pIntelligence->GetTaskJetPack();
            if (pTask && pTask->m_pJetPackClump) {
                ProcessNodesRecursive((RwFrame *)pTask->m_pJetPackClump->object.parent, pPed, eModelEntityType::Weapon);
            }

            // weapons
            CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
            if (pWeapon) {
                eWeaponType weaponType = pWeapon->m_eWeaponType;
                CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
                if (pWeaponInfo) {
                    CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
                    if (pWeaponModelInfo && pWeaponModelInfo->m_pRwClump) {
                        ProcessNodesRecursive((RwFrame *)pWeaponModelInfo->m_pRwClump->object.parent, pWeapon, eModelEntityType::Weapon);
                    }
                }
            }
        };
    }
    return TRUE;
}

