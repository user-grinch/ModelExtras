#include "lights/loader.h"
#include "pch.h"
#include <plugin.h>
#include <CHud.h>
#include <CMessages.h>
#include <shared/extensions/ScriptCommands.h>

#include "defines.h"
#include "loader.h"
#include "features/chain.h"
#include "features/gauge.h"
#include "features/handlebar.h"
#include "features/steerwheel.h"
#include "features/spotlights.h"
#include "features/wheelhub.h"
#include "features/remap.h"
#include "features/lights.h"
#include "features/sirens.h"
#include "features/plate.h"
#include "features/carcols.h"
#include "utils/datamgr.h"
#include "utils/audiomgr.h"
#include "utils/modelinfomgr.h"
#include "features/soundeffects.h"
#include "features/spoiler.h"
#include "features/dirtfx.h"
#include "features/backfire.h"
#include "features/slidedoor.h"
#include "features/rotatedoor.h"
#include "features/pedcols.h"
#include "features/clock.h"
#include "features/exhausts.h"
#include "features/roof.h"
#include "features/leds.h"
#include "features/wheel.h"
#include "features/rollbackbed.h"
#include "utils/frameextention.h"
#include "utils/meevents.h"

void InitLogFile();

constexpr uint32_t TEST_CHEAT = 0x0ADC;

void ModelExtrasLoader::Init()
{
    AudioMgr::Init();
    ModelInfoMgr::Init();
    RwFrameExtension::Init();

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
            if (pVeh && plugin::Command<TEST_CHEAT>("MERELOAD"))
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
    new Remap();
    new PedColors();
    new HandleBar();
    new ChainFeature();
    new SlideDoor();
    new RotateDoor();
    new FixedGauge();
    new GearIndicator();
    new MileageIndicator();
    new RPMGauge();
    new SpeedGauge();
    new Spoiler();
    new TurboGauge();
    new BackFireEffect();
    new ConvertibleRoof();
    new DashboardLEDs();
    new DigitalClockFeature();
    new DirtFx();
    new ExhaustFx();
    new ExtraWheel();
    new LicensePlate();
    if (GetModuleHandle("SAMP.asi") == nullptr) {
        new Carcols();
    }
    new RollbackBed();
    new SteerWheel();
    new WheelHub();
    new Lights();
    new LightsFeature();
    new Sirens();
    new SoundEffects();
    new SpotLights();
    for (auto *pFeature : m_Features)
    {
        // if (pFeature->IsActive())
        // {
            pFeature->Init();
        // }StandardLights
    }
}

void ModelExtrasLoader::Reload(CVehicle *pVeh)
{
    for (auto* pFeature : m_Features) {
        if (pFeature) {
            pFeature->Reload(pVeh);
        }
    }
    ModelInfoMgr::Reload(pVeh);
    CHud::SetHelpMessage("Config reloaded", false, false, true);
}