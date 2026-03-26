#include "pch.h"
#include "roof.h"
#include "modelinfomgr.h"
#include "datamgr.h"
#include "audiomgr.h"
#include "utils/util.h"
#include "CWeather.h"

using namespace plugin;

bool ConvertibleRoof::UpdateRotation(RoofConfig &config, CVehicle *pVeh, bool closed)
{
    if (config.pFrame)
    {
        VehData &data = xData.Get(pVeh);
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

void ConvertibleRoof::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
                                    std::string name = GetFrameNodeName(pFrame);
                                    bool isBoot = name.starts_with("x_convertible_boot");
                                    bool isRoof = name.starts_with("x_convertible_roof");
                                    if (!isRoof && !isBoot)
                                    {
                                        return;
                                    }

                                    RoofConfig c;
                                    c.pFrame = pFrame;
                                    auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
                                    if (jsonData.contains("roofs") && jsonData["roofs"].contains(name))
                                    {
                                        auto &data = jsonData["roofs"][name];
                                        c.targetRot = jsonData["roofs"][name].value("rotation", c.targetRot);
                                        c.speed = jsonData["roofs"][name].value("speed", c.speed);
                                    }

                                    VehData &data = xData.Get(pVeh);
                                    data.m_bInit = true;

                                    // Randomly open the roofs
                                    if (isRoof)
                                    {
                                        if (!data.m_bRoofTargetExpanded && CWeather::NewWeatherType != eWeatherType::WEATHER_RAINY_SF && CWeather::OldWeatherType != eWeatherType::WEATHER_RAINY_SF && CWeather::NewWeatherType != eWeatherType::WEATHER_RAINY_COUNTRYSIDE && CWeather::OldWeatherType != eWeatherType::WEATHER_RAINY_COUNTRYSIDE)
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
        if (CWeather::NewWeatherType != eWeatherType::WEATHER_RAINY_SF && CWeather::OldWeatherType != eWeatherType::WEATHER_RAINY_SF && CWeather::NewWeatherType != eWeatherType::WEATHER_RAINY_COUNTRYSIDE && CWeather::OldWeatherType != eWeatherType::WEATHER_RAINY_COUNTRYSIDE)
        {
            return;
        }

        VehData &data = xData.Get(pVeh);
        if (data.m_bInit && !data.m_bRoofTargetExpanded && pVeh->m_pDriver && !pVeh->IsDriver(FindPlayerPed()))
        {
            data.m_bRoofTargetExpanded = true;
        }
    };

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen()) {
            return;
        }

        VehData& data = xData.Get(pVeh);
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
        size_t now = CTimer::m_snTimeInMilliseconds;
        static size_t prev = 0;
        static uint32_t roofToggleKey = gConfig.ReadInteger("KEYS", "RoofToggleKey", VK_R);

        if (KeyPressed(roofToggleKey) && now - prev > 500.0f)
        {
            CVehicle *pVeh = FindPlayerVehicle();
            if (pVeh)
            {
                VehData &data = xData.Get(pVeh);

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
