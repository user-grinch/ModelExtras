#include "pch.h"
#include "defines.h"
#include "mgr.h"
#include "vehicle/chain.h"
#include "vehicle/brakes.h"
#include "vehicle/gear.h"
#include "vehicle/gauge.h"
#include "vehicle/handlebar.h"
#include "vehicle/steerwheel.h"
#include "vehicle/spotlights.h"
#include "vehicle/wheelhub.h"
#include "weapon/bodystate.h"
#include "ped/remap.h"
#include "common/remap.h"
#include "vehicle/lights.h"
#include "vehicle/sirens.h"
#include "vehicle/plate.h"
#include "vehicle/carcols.h"
#include "datamgr.h"
#include "audiomgr.h"
#include "modelinfomgr.h"
#include "vehicle/soundeffects.h"
#include "vehicle/spoiler.h"
#include "vehicle/dirtfx.h"
#include "vehicle/backfire.h"
#include "utils/meevents.h"
#include <extensions/ScriptCommands.h>
#include <CHud.h>
#include <CMessages.h>
#include "vehicle/slidedoor.h"
#include "vehicle/rotatedoor.h"
#include "ped/ganghands.h"
#include "ped/pedcols.h"
#include "vehicle/clock.h"
#include "vehicle/exhausts.h"
#include "vehicle/roof.h"
#include "vehicle/leds.h"
#include "vehicle/wheel.h"
#include "vehicle/rollbackbed.h"


#include "utils/frameextention.h"

void InitLogFile();

constexpr uint32_t TEST_CHEAT = 0x0ADC;

void FeatureMgr::Initialize()
{
    ModelInfoMgr::Initialize();
    RwFrameExtension::Initialize();

    Events::initGameEvent.after += []()
    {
        DataMgr::Init();
        gbVehIKInstalled = GetModuleHandle("VehIK.asi") != nullptr;

        if (gbVehIKInstalled)
        {
            LOG(INFO) << "VehIK detected, disabling SteerWheel and HandleBar features.";
        }
    };

    if (gConfig.ReadBoolean("CONFIG", "DeveloperMode", false))
    {
        Events::processScriptsEvent += []()
        {
            CVehicle *pVeh = FindPlayerVehicle(-1, false);
            if (pVeh && Command<TEST_CHEAT>("MERELOAD"))
            {
                Reload(pVeh);
            }
        };
    }

    MEEvents::vehRenderEvent.before += [](CVehicle *pVeh)
    {
        if (GetModuleHandle("SilentPatchSA.asi") == nullptr)
        {
            static std::string text = "ModelExtras requires SilentPatchSA installed!";
            CMessages::AddMessageWithString((char *)text.c_str(), 5000, false, nullptr, true);
            LOG(WARNING) << text;
        }
    };

    Events::pedRenderEvent.before += [](CPed *pPed)
    {
        Add(static_cast<void *>(pPed), (RwFrame *)pPed->m_pRwClump->object.parent, eModelEntityType::Ped);
        Process(static_cast<void *>(pPed), eModelEntityType::Ped);

        // jetpack
        CTaskSimpleJetPack *pTask = pPed->m_pIntelligence->GetTaskJetPack();
        if (pTask && pTask->m_pJetPackClump)
        {
            Add(static_cast<void *>(&pPed->m_aWeapons[pPed->m_nSelectedWepSlot]), (RwFrame *)pTask->m_pJetPackClump->object.parent, eModelEntityType::Jetpack);
            Process(static_cast<void *>(&pPed->m_aWeapons[pPed->m_nSelectedWepSlot]), eModelEntityType::Jetpack);
        }

        // weapons
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nSelectedWepSlot];
        if (pWeapon)
        {
            eWeaponType weaponType = pWeapon->m_eWeaponType;
            CWeaponInfo *pWeaponInfo = CWeaponInfo::GetWeaponInfo(weaponType, pPed->GetWeaponSkill(weaponType));
            if (pWeaponInfo && pWeaponInfo->m_nModelId > 0)
            {
                auto *pWeaponModelInfo = reinterpret_cast<CWeaponModelInfo *>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId));
                if (pWeaponModelInfo && pWeaponModelInfo->m_pRwClump)
                {
                    Add(static_cast<void *>(&pPed->m_aWeapons[pPed->m_nSelectedWepSlot]),
                        (RwFrame *)pWeaponModelInfo->m_pRwClump->object.parent, eModelEntityType::Weapon);
                    Process(static_cast<void *>(&pPed->m_aWeapons[pPed->m_nSelectedWepSlot]), eModelEntityType::Weapon);
                }
            }
        }
    };

    Events::pedDtorEvent += [](CPed *ptr)
    {
        Remove(static_cast<void *>(ptr), eModelEntityType::Ped);
    };

    static ThiscallEvent<AddressList<0x5343B2, H_CALL>, PRIORITY_BEFORE, ArgPickN<CObject *, 0>, void(CObject *)> objectRenderEvent;
    objectRenderEvent.before += [](CObject *pObj)
    {
        Add(static_cast<void *>(pObj), (RwFrame *)pObj->m_pRwClump->object.parent, eModelEntityType::Object);
        Process(static_cast<void *>(pObj), eModelEntityType::Object);
    };

    Events::objectDtorEvent += [](CObject *ptr)
    {
        Remove(static_cast<void *>(ptr), eModelEntityType::Object);
    };

    AudioMgr::Initialize();

    // Common Section
    if (gConfig.ReadBoolean("CONFIG", "ModelVersionCheck", true))
    {
        Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model)
        {
            auto &jsonData = DataMgr::Get(model);
            if (jsonData.contains("Metadata"))
            {
                auto &info = jsonData["Metadata"];
                int ver = info.value("MinVer", MOD_VERSION_NUMBER);
                if (ver > MOD_VERSION_NUMBER)
                {
                    static std::string text;
                    text = std::format("Model {} requires ModelExtras v{} but v{} is installed.", model, ver, MOD_VERSION_NUMBER);
                    CMessages::AddMessageWithString(std::remove_const_t<char*>(text.c_str()), 5000, false, nullptr, true);
                    LOG(WARNING) << text;
                }
            }
        };
    }
    
    // Common Section
    if (gConfig.ReadBoolean("COMMON_FEATURES", "TextureRemaper", false))
    {
        PedRemap::Initialize();
        Remap::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::TextureRemapper)));
    }

    // Bikes Section
    if (gConfig.ReadBoolean("PED_FEATURES", "HDGangHands", false))
    {
        GangHands::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::GangHands)));
    }

    if (gConfig.ReadBoolean("PED_FEATURES", "PedCols", false))
    {
        PedColors::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::PedCols)));
    }

    // Bikes Section
    if (gConfig.ReadBoolean("BIKE_FEATURES", "AnimatedBrakes", false))
    {
        FrontBrake::Initialize();
        RearBrake::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedBrakes)));
    }

    if (gConfig.ReadBoolean("BIKE_FEATURES", "AnimatedClutch", false))
    {
        Clutch::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedClutch)));
    }

    if (gConfig.ReadBoolean("BIKE_FEATURES", "AnimatedGearLever", false))
    {
        GearLever::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedGearLever)));
    }

    if (gConfig.ReadBoolean("BIKE_FEATURES", "RotatingHandleBar", false))
    {
        HandleBar::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::RotatingHandleBar)));
    }

    // Vehicle Section
    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedChain", false))
    {
        ChainFeature::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedChain)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedDoors", false))
    {
        SlideDoor::Initialize();
        RotateDoor::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedDoors)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedGasMeter", false))
    {
        FixedGauge::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedGasMeter)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedGearMeter", false))
    {
        GearIndicator::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedGearMeter)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedOdoMeter", false))
    {
        MileageIndicator::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedOdoMeter)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedRpmMeter", false))
    {
        RPMGauge::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedRpmMeter)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedSpeedMeter", false))
    {
        SpeedGauge::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedSpeedMeter)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedSpoiler", false))
    {
        Spoiler::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedSpoiler)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedTurboMeter", false))
    {
        TurboGauge::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::AnimatedTurboMeter)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "BackfireEffect", false))
    {
        BackFireEffect::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::BackfireEffect)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "ConvertibleRoof", false))
    {
        ConvertibleRoof::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::ConvertibleRoof)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "DashboardLED", false))
    {
        DashboardLEDs::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::DashboardLED)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "DigitalClock", false))
    {
        DigitalClockFeature::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::Clock)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "DirtFX", false))
    {
        DirtFx::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::DirtFX)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "ExhaustFx", false))
    {
        ExhaustFx::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::ExhaustFx)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "ExtraWheels", false))
    {
        ExtraWheel::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::ExtraWheels)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "HDLicensePlate", false))
    {
        LicensePlate::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::HDLicensePlate)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "IVFCarcols", false) && GetModuleHandle("SAMP.asi") == nullptr)
    {
        IVFCarcols::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::IVFCarcols)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "RollbackBed", false))
    {
        RollbackBed::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::RollbackBed)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "RotatingSteeringWheel", false))
    {
        SteerWheel::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::RotatingSteeringWheel)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "RotatingWheelHubs", false))
    {
        WheelHub::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::RotatingWheelHubs)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights", false))
    {
        Lights::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::StandardLights)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "SirenLights", false))
    {
        Sirens::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::SirenLights)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects", false))
    {
        SoundEffects::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::SoundEffects)));
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "SpotLights", false))
    {
        SpotLights::Initialize();
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::SpotLights)));
    }

    // Weapons Section
    if (gConfig.ReadBoolean("WEAPON_FEATURES", "BodyStateVariation", false))
    {
        m_FunctionTable["x_body_state"] = BodyState::Process;
        m_bEnabledFeatures.set(static_cast<int>((eFeatureMatrix::BodyStateVariation)));
    }
}

void FeatureMgr::Reload(CVehicle *pVeh)
{
    Lights::Reload(pVeh);
    Sirens::Reload(pVeh);
    ModelInfoMgr::Reload(pVeh);
    ExhaustFx::Reload();
    CHud::SetHelpMessage("Config reloaded", false, false, true);
}

void FeatureMgr::FindNodes(void *ptr, RwFrame *frame, eModelEntityType type)
{
    if (frame != nullptr)
    {
        const std::string name = GetFrameNodeName(frame);
        for (auto e : m_FunctionTable)
        {
            if (STR_FOUND(name, e.first))
            {
                m_EntityTable[type][ptr].emplace_back(frame, e.first);
                LOG_VERBOSE("Found {} in model {}", e.first, static_cast<CEntity *>(ptr)->m_nModelIndex);
            }
        }

        if (RwFrame *newFrame = frame->child)
        {
            FindNodes(ptr, newFrame, type);
        }
        if (RwFrame *newFrame = frame->next)
        {
            FindNodes(ptr, newFrame, type);
        }
    }
}

void FeatureMgr::Add(void *ptr, RwFrame *frame, eModelEntityType type)
{
    if (!m_EntityTable[type].contains(ptr))
    {
        FindNodes(ptr, frame, type);
    }
}

void FeatureMgr::Remove(void *ptr, eModelEntityType type)
{
    m_EntityTable[type].erase(ptr);
}

void FeatureMgr::Process(void *ptr, eModelEntityType type)
{
    for (const auto& pEnt : m_EntityTable[type][ptr])
    {
        if (m_FunctionTable[pEnt.id])
        {
            m_FunctionTable[pEnt.id](ptr, pEnt.m_pFrame, type);
        }
    }
}