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

static ThiscallEvent <AddressList<0x5343B2, H_CALL>, PRIORITY_BEFORE, ArgPickN<CObject*, 0>, void(CObject*)> objectRenderEvent;

void FeatureMgr::Initialize() {
    plugin::Events::vehicleRenderEvent.before += [](CVehicle* vehicle) {
        VehicleMaterials::RestoreMaterials();
        VehicleMaterials::OnRender(vehicle);
    };

    plugin::Events::vehicleSetModelEvent += VehicleMaterials::OnModelSet;

    Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model) {
        Add(static_cast<void*>(pVeh), (RwFrame *)pVeh->m_pRwClump->object.parent, eModelEntityType::Vehicle);
    };

    Events::vehicleRenderEvent.before += [](CVehicle *pVeh) {
        Process(static_cast<void*>(pVeh), eModelEntityType::Vehicle);
    };

    Events::pedRenderEvent += [](CPed* pPed) {
        Add(static_cast<void*>(pPed), 
            (RwFrame *)pPed->m_pRwClump->object.parent, eModelEntityType::Ped);
        Process(static_cast<void*>(pPed), eModelEntityType::Ped);

        // jetpack
        CTaskSimpleJetPack *pTask = pPed->m_pIntelligence->GetTaskJetPack();
        if (pTask && pTask->m_pJetPackClump) {
            Add(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), 
                (RwFrame *)pTask->m_pJetPackClump->object.parent, eModelEntityType::Jetpack);
            Process(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), eModelEntityType::Jetpack);
        }

        // weapons
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            eWeaponType weaponType = pWeapon->m_eWeaponType;
            CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
            if (pWeaponInfo && pWeaponInfo->m_nModelId1 > 0) {
                CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
                if (pWeaponModelInfo && pWeaponModelInfo->m_pRwClump) {
                    Add(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), 
                        (RwFrame *)pWeaponModelInfo->m_pRwClump->object.parent, eModelEntityType::Weapon);
                    Process(static_cast<void*>(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]), eModelEntityType::Weapon);
                }
            }
        }
    };

    objectRenderEvent += [](CObject *pObj) {
        Add(static_cast<void*>(pObj), 
            (RwFrame *)pObj->m_pRwClump->object.parent, eModelEntityType::Object);
        Process(static_cast<void*>(pObj), eModelEntityType::Object);
    };
    
    // Index features
    m_FunctionTable["x_chain"] = m_FunctionTable["fc_chain"] = Chain::Process;
    m_FunctionTable["x_fbrake"] = m_FunctionTable["fc_fbrake"] = FrontBrake::Process;
    m_FunctionTable["x_rbrake"] = m_FunctionTable["fc_rbrake"] = RearBrake::Process;
    m_FunctionTable["x_clutch"] = m_FunctionTable["fc_cl"] = Clutch::Process;
    m_FunctionTable["x_gearlever"] = m_FunctionTable["fc_gl"] = GearLever::Process;
    m_FunctionTable["x_gs"] = GearSound::Process;
    m_FunctionTable["x_gearmeter"] = m_FunctionTable["fc_gm"] = GearMeter::Process;
    m_FunctionTable["x_ometer"] = m_FunctionTable["fc_om"] = OdoMeter::Process;
    m_FunctionTable["x_rpm"] = m_FunctionTable["fc_rpm"] = RpmMeter::Process;
    m_FunctionTable["x_sm"] = m_FunctionTable["fc_sm"] = m_FunctionTable["speedook"] = SpeedMeter::Process;
    m_FunctionTable["x_tm"] = m_FunctionTable["tahook"] = TachoMeter::Process;
    m_FunctionTable["x_gm"] = m_FunctionTable["petrolok"] = GasMeter::Process;

    if (gConfig.ReadBoolean("FEATURES", "RotateHandleBars", false)) {
        m_FunctionTable["forks_front"] = HandleBar::AddSource;
        m_FunctionTable["handlebars"] = HandleBar::Process;
    }

    if (gConfig.ReadBoolean("FEATURES", "RotateSteerWheel", false)) {
        // TODO: need updated IDs
        m_FunctionTable["steer"] = SteerWheel::Process;
        m_FunctionTable["steering_dummy"] = SteerWheel::Process;
    }

    m_FunctionTable["spotlight_dummy"] = SpotLight::Process;
    m_FunctionTable["hub_"] = WheelHub::Process;

    Lights::Initialize();
    Sirens::Initialize();
    PaintJobs::Initialize();
    Remap::Initialize();
    Randomizer::Initialize();
    WeaponSoundSystem::Initialize();

    m_FunctionTable["x_body_state"] = BodyState::Process;
    m_FunctionTable["x_remap"] = BloodRemap::Process;
    m_FunctionTable["x_randomizer"] = Randomizer::Process;
}

static std::string GetNodeName(const std::string& input) {
    int c = 0;
    std::string result;
    
    for (char c : input) {
        if (c == '_') {
            c++;
            if (c == 2) {
                break;
            }
        }
        result += c;
    }
    
    return result;
}

void FeatureMgr::FindNodes(void *ptr, RwFrame * frame, eModelEntityType type) {
    if(frame) {
        int model = 0;
        if (type == eModelEntityType::Weapon) {
            model = static_cast<CWeapon*>(ptr)->m_eWeaponType;
        } else {
            model = static_cast<CEntity*>(ptr)->m_nModelIndex;
        }
        const std::string name = GetFrameNodeName(frame);

        for (auto e : m_FunctionTable) {
            if (NODE_FOUND(name, e.first)) {
                m_ModelTable[model].emplace_back(frame, e.first);
            }
        }

        // if (m_FunctionTable.find(GetNodeName(name)) != m_FunctionTable.end()) {
        //     m_ModelTable[pEntity->m_nModelIndex].emplace_back(frame, name);
        // }

        if (RwFrame * newFrame = frame->child) {
            FindNodes(ptr, newFrame, type);
        }
        if (RwFrame * newFrame = frame->next) {
            FindNodes(ptr, newFrame, type);
        }
    }
    return;
}

void FeatureMgr::Add(void *ptr, RwFrame* frame, eModelEntityType type) {
    int model = 0;
    if (type == eModelEntityType::Weapon) {
        model = static_cast<CWeapon*>(ptr)->m_eWeaponType;
    } else if (type == eModelEntityType::Jetpack) { 
        model = 370;
    } else {
        model = static_cast<CEntity*>(ptr)->m_nModelIndex;
    }

    if (m_ModelTable.find(model) == m_ModelTable.end()) {
        FindNodes(ptr, frame, type);
    }
}

void FeatureMgr::Process(void *ptr, eModelEntityType type) {
    int model = 0;
    if (type == eModelEntityType::Weapon) {
        model = static_cast<CWeapon*>(ptr)->m_eWeaponType;
    } else if (type == eModelEntityType::Jetpack) { 
        model = 370;
    } else {
        model = static_cast<CEntity*>(ptr)->m_nModelIndex;
    }

    for (auto e: m_ModelTable[model]) {
        if (m_FunctionTable[e.id]) {
            m_FunctionTable[e.id](ptr, e.m_pFrame, type);
        }
    }
}