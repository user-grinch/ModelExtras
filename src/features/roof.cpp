#include "pch.h"
#include "roof.h"
#include "utils/modelinfomgr.h"
#include "utils/datamgr.h"
#include "utils/audiomgr.h"
#include "utils/util.h"
#include "CWeather.h"

using namespace plugin;

bool ConvertibleRoof::UpdateRotation(RoofConfig &config, CVehicle *pVeh, bool closed)
{
    if (config.pFrame)
    {
        RoofData &data = m_VehData.Get(pVeh);
        MatrixUtil::SetRotationXAbsolute(&config.pFrame->modelling, config.currentRot - config.prevRot);
        config.prevRot = config.currentRot;

        float target = closed ? 0.0f : config.targetRot;
        float delta = target - config.currentRot;
        float step = CTimer::ms_fTimeStep * std::abs(config.targetRot) / 360.0f * config.speed;

        if (std::abs(delta) > step)
        {
            config.currentRot += step * (delta > 0.0f ? 1.0f : -1.0f);
        }
        else
        {
            config.currentRot = target;
            return true;
        }
    }
    return false;
}

static uint32_t g_nRoofToggleKey = 'T';

void ConvertibleRoof::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    g_nRoofToggleKey = gConfig.ReadInteger("KEYS", "RoofToggleKey", 'T');
}

void ConvertibleRoof::Reload(CVehicle *pVeh)
{
    ReloadConfig();
}

void ConvertibleRoof::Init()
{
    ReloadConfig();
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        bool isBoot = nodeName.starts_with("x_convertible_boot");
        bool isRoof = nodeName.starts_with("x_convertible_roof");
        if (!isRoof && !isBoot)
        {
            return;
        }

        RoofConfig c;
        c.pFrame = pFrame;
        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        std::string name(nodeName);
        if (jsonData.contains("roofs") && jsonData["roofs"].contains(name))
        {
            auto &data = jsonData["roofs"][name];
            c.targetRot = data.value("rotation", c.targetRot);
            c.speed = data.value("speed", c.speed);
        }

        RoofData &data = m_VehData.Get(pVeh);
        data.m_bInit = true;

                                    // Randomly open the roofs
                                    if (isRoof)
                                    {
                                        bool isRainy = (CWeather::Rain > 0.05f) || (CWeather::WetRoads > 0.1f) || (CWeather::NewWeatherType == eWeatherType::WEATHER_RAINY_SF || CWeather::OldWeatherType == eWeatherType::WEATHER_RAINY_SF || CWeather::NewWeatherType == eWeatherType::WEATHER_RAINY_COUNTRYSIDE || CWeather::OldWeatherType == eWeatherType::WEATHER_RAINY_COUNTRYSIDE);
                                        if (!data.m_bRoofTargetExpanded && !isRainy)
                                        {
                                            MatrixUtil::SetRotationXAbsolute(&pFrame->modelling, c.targetRot - c.prevRot);
                                            c.prevRot = c.targetRot;
                                            c.currentRot = c.targetRot;
                                        }
                                        data.m_Roofs.push_back(std::move(c));
                                    }
                                    else
                                    {
                                        data.m_Boots.push_back(std::move(c));
                                    } });

    Events::vehicleRenderEvent += [](CVehicle *pVeh)
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::ConvertibleRoof)) return;
        bool isRainy = (CWeather::Rain > 0.05f) || (CWeather::WetRoads > 0.1f) || (CWeather::NewWeatherType == eWeatherType::WEATHER_RAINY_SF || CWeather::OldWeatherType == eWeatherType::WEATHER_RAINY_SF || CWeather::NewWeatherType == eWeatherType::WEATHER_RAINY_COUNTRYSIDE || CWeather::OldWeatherType == eWeatherType::WEATHER_RAINY_COUNTRYSIDE);
        if (!isRainy)
        {
            return;
        }

        RoofData &data = m_VehData.Get(pVeh);
        if (data.m_bInit && !data.m_bRoofTargetExpanded && pVeh->m_pDriver && !pVeh->IsDriver(FindPlayerPed()))
        {
            data.m_bRoofTargetExpanded = true;
        }
    };

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::ConvertibleRoof)) return;
        if (!pVeh || !pVeh->GetIsOnScreen()) {
            return;
        }

        RoofData& data = m_VehData.Get(pVeh);
        if (!data.m_bInit)  {
            return;
        }

        if (data.m_bRoofTargetExpanded != data.m_bPrevTarget && data.m_phase == AnimPhase::Idle) {
            data.m_phase = AnimPhase::OpeningBoots;
        }

        switch (data.m_phase) {
            case AnimPhase::OpeningBoots: {
                bool allOpened = true;
                for (auto& panel : data.m_Boots) {
                    if (!UpdateRotation(panel, pVeh, false)) {
                        allOpened = false;
                    }
                }
                if (allOpened) {
                    data.m_phase = AnimPhase::MovingRoof;
                }
                break;
            }

            case AnimPhase::MovingRoof: {
                bool allMoved = true;
                for (auto& roof : data.m_Roofs) {
                    if (!UpdateRotation(roof, pVeh, data.m_bRoofTargetExpanded)) {
                        allMoved = false;
                    }
                }
                if (allMoved) {
                    data.m_phase = AnimPhase::ClosingBoots;
                }
                break;
            }

            case AnimPhase::ClosingBoots: {
                bool allClosed = true;
                for (auto& panel : data.m_Boots) {
                    if (!UpdateRotation(panel, pVeh, true)) {
                        allClosed = false;
                    }
                }
                if (allClosed) {
                    data.m_phase = AnimPhase::Idle;
                    data.m_bPrevTarget = data.m_bRoofTargetExpanded;
                }
                break;
            }
            default:
                break;
        } });

    Events::processScriptsEvent += []()
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::ConvertibleRoof)) return;
        size_t now = CTimer::m_snTimeInMilliseconds;
        static size_t prev = 0;

        if (Util::IsKeyPressed(g_nRoofToggleKey) && now - prev > 500.0f)
        {
            CVehicle *pVeh = FindPlayerVehicle();
            if (pVeh)
            {
                RoofData &data = m_VehData.Get(pVeh);

                if (data.m_bInit)
                {
                    data.m_bRoofTargetExpanded = !data.m_bRoofTargetExpanded;
                    prev = now;
                    AudioMgr::PlaySwitchSound(pVeh);
                }
            }
        }
    };
}
