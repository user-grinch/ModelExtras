#include "pch.h"
#include "mgr.h"
#include "vehicle/chain.h"
#include "vehicle/brakes.h"
#include "vehicle/gear.h"
#include "vehicle/meter.h"
#include "vehicle/handlebar.h"
#include "vehicle/steerwheel.h"
#include "vehicle/spotlights.h"
#include "vehicle/wheelhub.h"
#include "weapon/bodystate.h"
#include "weapon/bloodremap.h"
#include "weapon/sound.h"
#include "common/remap.h"
#include "common/randomizer.h"
#include "vehicle/avs/common.h"
#include "vehicle/lights.h"
#include "vehicle/paintjobs.h"
#include "vehicle/avs/materials.h"
#include "vehicle/sirens.h"

void FeatureMgr::Initialize() {
    plugin::Events::vehicleRenderEvent.before += [](CVehicle* vehicle) {
        VehicleMaterials::RestoreMaterials();
        VehicleMaterials::OnRender(vehicle);
        };

    plugin::Events::vehicleSetModelEvent += VehicleMaterials::OnModelSet;

    Events::vehicleRenderEvent.before += [](CVehicle* pVeh) {
        Add(static_cast<void*>(pVeh), (RwFrame*)pVeh->m_pRwClump->object.parent, eModelEntityType::Vehicle);
        Process(static_cast<void*>(pVeh), eModelEntityType::Vehicle);
        };

    Events::vehicleDtorEvent += [](CVehicle* pVeh) {
        Remove(static_cast<void*>(pVeh), eModelEntityType::Vehicle);
        };

    Events::pedRenderEvent.before += [](CPed* pPed) {
        Add(static_cast<void*>(pPed), (RwFrame*)pPed->m_pRwClump->object.parent, eModelEntityType::Ped);
        Process(static_cast<void*>(pPed), eModelEntityType::Ped);

        // jetpack
        CTaskSimpleJetPack* pTask = pPed->m_pIntelligence->GetTaskJetPack();
        if (pTask && pTask->m_pJetPackClump) {
            Add(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), (RwFrame*)pTask->m_pJetPackClump->object.parent, eModelEntityType::Jetpack);
            Process(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), eModelEntityType::Jetpack);
        }

        // weapons
        CWeapon* pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            eWeaponType weaponType = pWeapon->m_eWeaponType;
            CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
            if (pWeaponInfo && pWeaponInfo->m_nModelId1 > 0) {
                CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
                if (pWeaponModelInfo && pWeaponModelInfo->m_pRwClump) {
                    Add(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]),
                        (RwFrame*)pWeaponModelInfo->m_pRwClump->object.parent, eModelEntityType::Weapon);
                    Process(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), eModelEntityType::Weapon);
                }
            }
        }
        };

    Events::pedDtorEvent += [](CPed* ptr) {
        Remove(static_cast<void*>(ptr), eModelEntityType::Ped);
        };

    static ThiscallEvent <AddressList<0x5343B2, H_CALL>, PRIORITY_BEFORE, ArgPickN<CObject*, 0>, void(CObject*)> objectRenderEvent;
    objectRenderEvent.before += [](CObject* pObj) {
        Add(static_cast<void*>(pObj), (RwFrame*)pObj->m_pRwClump->object.parent, eModelEntityType::Object);
        Process(static_cast<void*>(pObj), eModelEntityType::Object);
        };

    Events::objectDtorEvent += [](CObject* ptr) {
        Remove(static_cast<void*>(ptr), eModelEntityType::Object);
        };

        // Index features
    gLogger->info("Enabled features,");
    if (gConfig.ReadBoolean("FEATURES", "Chains", false)) {
        m_FunctionTable["x_chain"] = m_FunctionTable["fc_chain"] = Chain::Process;
        gLogger->info("Chain");
    }

    if (gConfig.ReadBoolean("FEATURES", "Brakes", false)) {
        m_FunctionTable["x_fbrake"] = m_FunctionTable["fc_fbrake"] = FrontBrake::Process;
        m_FunctionTable["x_rbrake"] = m_FunctionTable["fc_rbrake"] = RearBrake::Process;
        m_FunctionTable["x_clutch"] = m_FunctionTable["fc_cl"] = Clutch::Process;
        m_FunctionTable["x_gearlever"] = m_FunctionTable["fc_gl"] = GearLever::Process;
        gLogger->info("Brakes");
    }

    if (gConfig.ReadBoolean("FEATURES", "GearSounds", false)) {
        m_FunctionTable["x_gs"] = GearSound::Process;
        gLogger->info("Gear Sounds");
    }

    if (gConfig.ReadBoolean("FEATURES", "GearMeter", false)) {
        m_FunctionTable["x_gearmeter"] = m_FunctionTable["fc_gm"] = GearMeter::Process;
        gLogger->info("Gear Meter");
    }

    if (gConfig.ReadBoolean("FEATURES", "OdoMeter", false)) {
        m_FunctionTable["x_ometer"] = m_FunctionTable["fc_om"] = OdoMeter::Process;
        gLogger->info("OdoMeter");
    }

    if (gConfig.ReadBoolean("FEATURES", "RpmMeter", false)) {
        m_FunctionTable["x_rpm"] = m_FunctionTable["fc_rpm"] = m_FunctionTable["tahook"] = RpmMeter::Process;
        gLogger->info("RPM Meter");
    }
    if (gConfig.ReadBoolean("FEATURES", "SpeedMeter", false)) {
        m_FunctionTable["x_sm"] = m_FunctionTable["fc_sm"] = m_FunctionTable["speedook"] = SpeedMeter::Process;
        gLogger->info("Speed Meter");
    }
    if (gConfig.ReadBoolean("FEATURES", "GasMeter", false)) {
        m_FunctionTable["x_gm"] = m_FunctionTable["petrolok"] = GasMeter::Process;
        gLogger->info("Gas Meter");
    }

    if (gConfig.ReadBoolean("FEATURES", "RotateHandleBars", false)) {
        m_FunctionTable["forks_front"] = HandleBar::AddSource;
        m_FunctionTable["handlebars"] = HandleBar::Process;
        gLogger->info("Rotating Handlebars");
    }

    if (gConfig.ReadBoolean("FEATURES", "RotateSteerWheel", false)) {
        // TODO: need updated IDs
        m_FunctionTable["steer"] = SteerWheel::Process;
        m_FunctionTable["steering_dummy"] = SteerWheel::Process;
        gLogger->info("Steering");
    }

    if (gConfig.ReadBoolean("FEATURES", "SpotLights", false)) {
        m_FunctionTable["spotlight_dummy"] = SpotLight::Process;
        gLogger->info("Spotlights");
    }

    if (gConfig.ReadBoolean("FEATURES", "WheelHubs", false)) {
        m_FunctionTable["hub_"] = WheelHub::Process;
        m_FunctionTable["wheel_"] = WheelHub::Process;
        gLogger->info("WheelHubs");
    }

    if (gConfig.ReadBoolean("FEATURES", "Sirens", false)) {
        Sirens::Initialize();
        gLogger->info("Sirens");
    }
    if (gConfig.ReadBoolean("FEATURES", "Lights", false)) {
        Lights::Initialize();
        gLogger->info("Lights");
    }

    if (gConfig.ReadBoolean("FEATURES", "WeaponSoundSystem", false)) {
        WeaponSoundSystem::Initialize();
        gLogger->info("Weapon Sounds");
    }

    if (gConfig.ReadBoolean("FEATURES", "PaintJobs", false)) {
        PaintJobs::Initialize();
        gLogger->info("Paintjobs");
    }

    if (gConfig.ReadBoolean("FEATURES", "BodyStates", false)) {
        m_FunctionTable["x_body_state"] = BodyState::Process;
        gLogger->info("Body States");
    }

    if (gConfig.ReadBoolean("FEATURES", "Remap", false)) {
        Remap::Initialize();
        m_FunctionTable["x_remap"] = BloodRemap::Process;
        gLogger->info("Random Remap");
    }

    if (gConfig.ReadBoolean("FEATURES", "Randomizer", false)) {
        Randomizer::Initialize();
        m_FunctionTable["x_randomizer"] = Randomizer::Process;
        gLogger->info("Randomizer");
    }
    PRINT_LINEBREAK
}

void FeatureMgr::FindNodes(void* ptr, RwFrame* frame, eModelEntityType type) {
    if (frame) {
        const std::string name = GetFrameNodeName(frame);
        for (auto e : m_FunctionTable) {
            if (NODE_FOUND(name, e.first)) {
                m_EntityTable[type][ptr].emplace_back(frame, e.first);
                LOG_VERBOSE("Found {} in model {}", e.first, static_cast<CEntity*>(ptr)->m_nModelIndex);
            }
        }

        if (RwFrame* newFrame = frame->child) {
            FindNodes(ptr, newFrame, type);
        }
        if (RwFrame* newFrame = frame->next) {
            FindNodes(ptr, newFrame, type);
        }
    }
    return;
}

void FeatureMgr::Add(void* ptr, RwFrame* frame, eModelEntityType type) {
    if (m_EntityTable[type].find(ptr) == m_EntityTable[type].end()) {
        FindNodes(ptr, frame, type);
    }
}

void FeatureMgr::Remove(void* ptr, eModelEntityType type) {
    m_EntityTable[type].erase(ptr);
}

void FeatureMgr::Process(void* ptr, eModelEntityType type) {
    for (auto e : m_EntityTable[type][ptr]) {
        if (m_FunctionTable[e.id]) {
            m_FunctionTable[e.id](ptr, e.m_pFrame, type);
        }
    }
}