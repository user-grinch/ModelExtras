#include "pch.h"
#include "defines.h"
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
#include "weapon/sound.h"
#include "common/remap.h"
#include "common/randomizer.h"
#include "vehicle/lights.h"
#include "vehicle/paintjobs.h"
#include "vehicle/core/materials.h"
#include "vehicle/sirens.h"
#include "vehicle/plate.h"
#include "vehicle/carcols.h"
#include "datamgr.h"
#include "audiomgr.h"
#include "vehicle/soundeffects.h"
#include "vehicle/spoiler.h"
#include "vehicle/backfire.h"
#include "vehicle/dirt/modelinfo.h"
#include "meevents.h"
#include <extensions/ScriptCommands.h>
#include <CHud.h>

#include <CMessages.h>

void InitLogFile();

#define TEST_CHEAT 0x0ADC

void FeatureMgr::Initialize()
{
    ModelInfoMgr::Initialize();
    plugin::Events::initGameEvent += []()
    {
        DataMgr::Init();
    };

    if (gConfig.ReadBoolean("CONFIG", "DeveloperMode", false))
    {
        Events::processScriptsEvent += []()
        {
            CVehicle *pVeh = FindPlayerVehicle(-1, false);
            if (pVeh && plugin::Command<TEST_CHEAT>("MERELOAD"))
            {
                Lights::Reload(pVeh);
                // Sirens::Reload(pVeh);
                CHud::SetHelpMessage("Config reloaded", false, false, true);
            }
        };
    }

    MEEvents::vehRenderEvent.before += [](CVehicle *pVeh)
    {
        static bool spInstalled = GetModuleHandle("SilentPatchSA.asi");
        if (!spInstalled)
        {
            static std::string text = "ModelExtras requires SilentPatchSA installed!";
            CMessages::AddMessageWithString((char *)text.c_str(), 5000, false, NULL, true);
            gLogger->warn(text);
        }

        Process(static_cast<void *>(pVeh), eModelEntityType::Vehicle);
    };

    MEEvents::heliRenderEvent.after += [](CVehicle *pVeh)
    {
        if (!CModelInfo::IsHeliModel(pVeh->m_nModelIndex))
        {
            return;
        }

        Process(static_cast<void *>(pVeh), eModelEntityType::Vehicle);
    };

    Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model)
    {
        Add(static_cast<void *>(pVeh), (RwFrame *)pVeh->m_pRwClump->object.parent, eModelEntityType::Vehicle);
    };

    Events::vehicleDtorEvent += [](CVehicle *pVeh)
    {
        Remove(static_cast<void *>(pVeh), eModelEntityType::Vehicle);
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
                CWeaponModelInfo *pWeaponModelInfo = static_cast<CWeaponModelInfo *>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId));
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

    InitLogFile();
    AudioMgr::Initialize();

    // Common Section
    LOG_NO_LEVEL("\nCore->");
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
                    CMessages::AddMessageWithString((char *)text.c_str(), 5000, false, NULL, true);
                    gLogger->warn(text);
                }
            }
        };
        LOG_NO_LEVEL("  ModelVersionCheck");
    }

    // Common Section
    LOG_NO_LEVEL("\nCommon Features->");
    if (gConfig.ReadBoolean("COMMON_FEATURES", "TextureRemaper", false))
    {
        Remap::Initialize();
        LOG_NO_LEVEL("  TextureRemaper");
    }

    if (gConfig.ReadBoolean("COMMON_FEATURES", "ModelRandomizer", false))
    {
        Randomizer::Initialize();
        m_FunctionTable["x_randomizer"] = Randomizer::Process;
        LOG_NO_LEVEL("  ModelRandomizer");
    }

    // Bikes Section
    LOG_NO_LEVEL("\nBike Features->");
    if (gConfig.ReadBoolean("BIKE_FEATURES", "AnimatedBrakes", false))
    {
        m_FunctionTable["x_fbrake"] = m_FunctionTable["fc_fbrake"] = FrontBrake::Process;
        m_FunctionTable["x_rbrake"] = m_FunctionTable["fc_rbrake"] = RearBrake::Process;
        LOG_NO_LEVEL("  AnimatedBrakes");
    }

    if (gConfig.ReadBoolean("BIKE_FEATURES", "AnimatedClutch", false))
    {
        m_FunctionTable["x_clutch"] = m_FunctionTable["fc_cl"] = Clutch::Process;
        LOG_NO_LEVEL("  AnimatedClutch");
    }

    if (gConfig.ReadBoolean("BIKE_FEATURES", "AnimatedGearLever", false))
    {
        m_FunctionTable["x_gearlever"] = m_FunctionTable["fc_gl"] = GearLever::Process;
        LOG_NO_LEVEL("  AnimatedClutch");
    }

    if (gConfig.ReadBoolean("BIKE_FEATURES", "RotatingHandleBar", false))
    {
        HandleBar::Initialize();
        LOG_NO_LEVEL("  RotatingHandleBar");
    }

    // Vehicle Section
    LOG_NO_LEVEL("\nVehicle Features->");
    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AdditionalPaintJobs", false))
    {
        PaintJobs::Initialize();
        LOG_NO_LEVEL("  AdditionalPaintJobs");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedChain", false))
    {
        ChainFeature::Initialize();
        LOG_NO_LEVEL("  AnimatedChain");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedGasMeter", false))
    {
        m_FunctionTable["x_gm"] = m_FunctionTable["petrolok"] = GasMeter::Process;
        LOG_NO_LEVEL("  AnimatedGasMeter");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedGearMeter", false))
    {
        m_FunctionTable["x_gearmeter"] = m_FunctionTable["fc_gm"] = GearMeter::Process;
        LOG_NO_LEVEL("  AnimatedGearMeter");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedOdoMeter", false))
    {
        m_FunctionTable["x_ometer"] = m_FunctionTable["fc_om"] = OdoMeter::Process;
        LOG_NO_LEVEL("  AnimatedOdoMeter");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedRpmMeter", false))
    {
        m_FunctionTable["x_rpm"] = m_FunctionTable["fc_rpm"] = m_FunctionTable["tahook"] = RpmMeter::Process;
        LOG_NO_LEVEL("  AnimatedRpmMeter");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedSpeedMeter", false))
    {
        m_FunctionTable["x_sm"] = m_FunctionTable["fc_sm"] = m_FunctionTable["speedook"] = SpeedMeter::Process;
        LOG_NO_LEVEL("  AnimatedSpeedMeter");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "AnimatedSpoiler", false))
    {
        Spoiler::Initialize();
        LOG_NO_LEVEL("  AnimatedSpoiler");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "BackfireEffect", false))
    {
        BackFireEffect::Initialize(NULL, NULL);
        plugin::Events::vehicleRenderEvent.before += [](CVehicle *vehicle)
        {
            BackFireEffect::Process(vehicle, nullptr, eModelEntityType::Vehicle);
        };
        LOG_NO_LEVEL("  BackfireEffect");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "DirtFX", false))
    {
        FxVehModelInfo::Initialize();
        LOG_NO_LEVEL("  DirtFX");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "HDLicensePlate", false))
    {
        LicensePlate::Initialize();
        LOG_NO_LEVEL("  HDLicensePlate");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "IVFCarcols", false))
    {
        if (GetModuleHandle("SAMP.dll") || GetModuleHandle("SAMP.asi"))
        {
            LOG_NO_LEVEL("  IVFCarcols: SAMP detected, feature disabled. ")
        }
        else
        {
            IVFCarcols::Initialize();
            LOG_NO_LEVEL("  IVFCarcols");
        }
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "RotatingSteeringWheel", false))
    {
        SteerWheel::Initialize();
        LOG_NO_LEVEL("  RotatingSteeringWheel");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "RotatingWheelHubs", false))
    {
        WheelHub::Initialize();
        LOG_NO_LEVEL("  RotatingWheelHubs");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "StandardLights", false))
    {
        Lights::Initialize();
        LOG_NO_LEVEL("  StandardLights");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "SirenLights", false))
    {
        // Sirens::Initialize();
        LOG_NO_LEVEL("  SirenLights");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects", false))
    {
        SoundEffects::Initialize();
        LOG_NO_LEVEL("  SoundEffects");
    }

    if (gConfig.ReadBoolean("VEHICLE_FEATURES", "SpotLights", false))
    {
        SpotLights::Initialize();
        LOG_NO_LEVEL("  Spotlights");
    }

    // Weapons Section
    LOG_NO_LEVEL("\nWeapon Features->");
    if (gConfig.ReadBoolean("WEAPON_FEATURES", "BodyStateVariation", false))
    {
        m_FunctionTable["x_body_state"] = BodyState::Process;
        LOG_NO_LEVEL("  BodyStateVariation");
    }

    if (gConfig.ReadBoolean("WEAPON_FEATURES", "CustomSounds", false))
    {
        WeaponSoundSystem::Initialize();
        LOG_NO_LEVEL("  CustomSounds");
    }

    LOG_NO_LEVEL("");
}

void FeatureMgr::FindNodes(void *ptr, RwFrame *frame, eModelEntityType type)
{
    if (frame)
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
    return;
}

void FeatureMgr::Add(void *ptr, RwFrame *frame, eModelEntityType type)
{
    if (m_EntityTable[type].find(ptr) == m_EntityTable[type].end())
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
    for (auto e : m_EntityTable[type][ptr])
    {
        if (m_FunctionTable[e.id])
        {
            m_FunctionTable[e.id](ptr, e.m_pFrame, type);
        }
    }
}