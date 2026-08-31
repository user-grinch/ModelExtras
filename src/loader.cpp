#include "lights/lights.h"
#include "pch.h"
#include <plugin.h>
#include <CHud.h>
#include <CMessages.h>
#include <shared/extensions/ScriptCommands.h>

#include "defines.h"
#include "loader.h"
#include "features/chain.h"
#include "features/gauge.h"
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
#include "utils/frameextension.h"
#include "utils/meevents.h"
#include "utils/samp.h"

constexpr uint32_t TEST_CHEAT = 0x0ADC;

bool gbProperShadersDetected = false;

void ModelExtras::Init()
{
    AudioMgr::Init();
    ModelInfoMgr::Init();
    RwFrameExtension::Init();

    Events::initGameEvent.after += []()
    {
        DataMgr::Init();
        gbProperShadersDetected = GetModuleHandle("ProperShaders.asi") != nullptr;
        if (gbProperShadersDetected)
        {
            LOG(INFO) << "Proper Shaders detected, enabling compatibility mode for ModelExtras lights.";
        }

        if (SAMP::IsPresent())
        {
            LOG(INFO) << "SAMP detected, disabling Carcols feature.";
        }

        if (GetModuleHandle("SilentPatchSA.asi") == nullptr)
        {
            static std::string text = "ModelExtras requires SilentPatchSA installed!";
            LOG(WARNING) << text;
        }
    };

    if (gConfig.ReadBoolean("CONFIG", "EnableLiveReload", true))
    {
        Events::processScriptsEvent += []()
        {
            if (plugin::Command<TEST_CHEAT>("MERELOAD"))
            {
                Reload();
            }
        };
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
    RegisterFeature<Remap>();
    RegisterFeature<PedColors>();
    RegisterFeature<ChainFeature>();
    RegisterFeature<SlideDoor>();
    RegisterFeature<RotateDoor>();
    RegisterFeature<FixedGauge>();
    RegisterFeature<GearIndicator>();
    RegisterFeature<MileageIndicator>();
    RegisterFeature<RPMGauge>();
    RegisterFeature<SpeedGauge>();
    RegisterFeature<Spoiler>();
    RegisterFeature<TurboGauge>();
    RegisterFeature<BackFireEffect>();
    RegisterFeature<ConvertibleRoof>();
    RegisterFeature<DashboardLEDs>();
    RegisterFeature<DigitalClockFeature>();
    RegisterFeature<DirtFx>();
    RegisterFeature<ExhaustFx>();
    RegisterFeature<ExtraWheel>();
    RegisterFeature<LicensePlate>();
    if (!SAMP::IsPresent()) {
        RegisterFeature<Carcols>();
    }
    RegisterFeature<RollbackBed>();
    RegisterFeature<WheelHub>();
    RegisterFeature<Lights>();
    RegisterFeature<LightsFeature>();
    RegisterFeature<Sirens>();
    RegisterFeature<SoundEffects>();
    RegisterFeature<SpotLights>();
    for (const auto &pFeature : m_Features)
    {
        if (pFeature)
        {
            pFeature->Init();
        }
    }
}

void ModelExtras::Reload()
{
    gConfig.data.clear();
    gConfig.SetIniPath();
    gVerboseLogging = gConfig.ReadBoolean("CONFIG", "VerboseLogging", false);
    AudioMgr::ReloadConfig();
    for (const auto &pFeature : m_Features) {
        if (pFeature) {
            pFeature->ReloadConfig();
        }
    }
    for (CVehicle *pVeh : CPools::ms_pVehiclePool) {
        for (const auto &pFeature : m_Features) {
            if (pFeature) {
                pFeature->Reload(pVeh);
            }
        }
        ModelInfoMgr::Reload(pVeh);
    }
    static std::string msg = "~g~ModelExtras:~w~ Config reloaded";
    CMessages::AddMessageWithString(const_cast<char*>(msg.c_str()), 3000, false, nullptr, true);
    LOG(INFO) << "ModelExtras: Configuration reloaded successfully.";
}