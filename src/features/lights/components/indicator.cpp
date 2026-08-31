#include "pch.h"
#include "indicator.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/render.h"
#include "../damage.h"
#include <CPathFind.h>
#include "defines.h"

void IndicatorComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_INDICATOR_LEFT_REAR.ToInt()] = eMaterialType::IndicatorLightLeftRear;
    matMap[VEHCOL_INDICATOR_LEFT_SIDE.ToInt()] = eMaterialType::IndicatorLightLeftMiddle;
    matMap[VEHCOL_INDICATOR_LEFT_FRONT.ToInt()] = eMaterialType::IndicatorLightLeftFront;
    matMap[VEHCOL_INDICATOR_RIGHT_REAR.ToInt()] = eMaterialType::IndicatorLightRightRear;
    matMap[VEHCOL_INDICATOR_RIGHT_SIDE.ToInt()] = eMaterialType::IndicatorLightRightMiddle;
    matMap[VEHCOL_INDICATOR_RIGHT_FRONT.ToInt()] = eMaterialType::IndicatorLightRightFront;
}

eMaterialType IndicatorComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_INDICATOR_LEFT_REAR) return eMaterialType::IndicatorLightLeftRear;
    if (matCol == VEHCOL_INDICATOR_LEFT_SIDE) return eMaterialType::IndicatorLightLeftMiddle;
    if (matCol == VEHCOL_INDICATOR_LEFT_FRONT) return eMaterialType::IndicatorLightLeftFront;
    if (matCol == VEHCOL_INDICATOR_RIGHT_REAR) return eMaterialType::IndicatorLightRightRear;
    if (matCol == VEHCOL_INDICATOR_RIGHT_SIDE) return eMaterialType::IndicatorLightRightMiddle;
    if (matCol == VEHCOL_INDICATOR_RIGHT_FRONT) return eMaterialType::IndicatorLightRightFront;
    return eMaterialType::UnknownMaterial;
}

bool IndicatorComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("turnl_") || name.starts_with("indicator_")) {
        auto d = Util::GetCharsAfterPrefix(name, "turnl_", 2);
        if (!d) d = Util::GetCharsAfterPrefix(name, "indicator_", 2);
        if (!d) d = Util::GetCharsAfterPrefix(name, "turnl_", 1);
        if (!d) d = Util::GetCharsAfterPrefix(name, "indicator_", 1);
        if (d) {
            DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
            bool isLeft = (d.value()[0] == 'L' || d.value()[0] == 'l');
            c.corona.color = c.shadow.color = {255, 128, 0, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
            c.corona.lightingType = eLightingMode::Directional;

            char posChar = (d.value().length() >= 2) ? d.value()[1] : (c.position.y >= 0.0f ? 'F' : 'R');
            switch (posChar) {
                case 'F':
                case 'f':
                    c.lightType = isLeft ? eMaterialType::IndicatorLightLeftFront : eMaterialType::IndicatorLightRightFront;
                    c.dummyPos = eDummyPos::Front;
                    break;
                case 'R':
                case 'r':
                    c.lightType = isLeft ? eMaterialType::IndicatorLightLeftRear : eMaterialType::IndicatorLightRightRear;
                    c.dummyPos = eDummyPos::Rear;
                    break;
                case 'M':
                case 'm':
                    c.lightType = isLeft ? eMaterialType::IndicatorLightLeftMiddle : eMaterialType::IndicatorLightRightMiddle;
                    c.dummyPos = isLeft ? eDummyPos::Right : eDummyPos::Left;
                    break;
                default:
                    c.lightType = isLeft ? eMaterialType::IndicatorLightLeftFront : eMaterialType::IndicatorLightRightFront;
                    c.dummyPos = eDummyPos::Front;
                    break;
            }
            data.dummies[c.lightType].push_back(new VehicleDummy(c));
            return true;
        }
    }
    return false;
}

struct CarPathLinkAddress {
    unsigned short m_nCarPathLinkId : 10;
    unsigned short m_nAreaId : 6;

    constexpr static auto* Cast(CCarPathLinkAddress* oldFormat) {
        return (CarPathLinkAddress*)(oldFormat);
    }
    constexpr static const auto* Cast(const CCarPathLinkAddress* oldFormat) {
        return (const CarPathLinkAddress*)(oldFormat);
    }
};

static CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    auto* addr = CarPathLinkAddress::Cast(&address);
    if (ThePaths.m_pNaviNodes && addr->m_nAreaId < 64 && ThePaths.m_pNaviNodes[addr->m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[addr->m_nAreaId][addr->m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
                         static_cast<float>(ThePaths.m_pNaviNodes[addr->m_nAreaId][addr->m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

void IndicatorComponent::Process(CVehicle* pVeh, VehLightData& data) {
    static bool bSAMP = GetModuleHandle("samp.dll") != nullptr;

    if (pVeh->m_pDriver == FindPlayerPed() &&
        (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE || pVeh->m_nVehicleSubClass == VEHICLE_QUAD || pVeh->m_nVehicleSubClass == VEHICLE_MTRUCK))
    {
        if (Util::IsKeyPressed(LightsConfig::Get().nIndicatorNoneKey)) {
            data.nIndicatorState = eIndicatorState::Off;
            BlinkerState::Get().Reset();
        } else if (Util::IsKeyPressed(LightsConfig::Get().nIndicatorLeftKey)) {
            data.nIndicatorState = eIndicatorState::LeftOn;
        } else if (Util::IsKeyPressed(LightsConfig::Get().nIndicatorRightKey)) {
            data.nIndicatorState = eIndicatorState::RightOn;
        } else if (Util::IsKeyPressed(LightsConfig::Get().nIndicatorBothKey)) {
            data.nIndicatorState = eIndicatorState::BothOn;
        }

        if (LightsConfig::Get().bAutoIndicatorsOnSteer && data.nIndicatorState != eIndicatorState::BothOn) {
            bool bSteerLeft = (pVeh->m_fSteerAngle > 0.08f) || Util::IsKeyPressed('A') || Util::IsKeyPressed(VK_LEFT);
            bool bSteerRight = (pVeh->m_fSteerAngle < -0.08f) || Util::IsKeyPressed('D') || Util::IsKeyPressed(VK_RIGHT);

            if (bSteerLeft && !bSteerRight) {
                data.nIndicatorState = eIndicatorState::LeftOn;
                data.bWasAutoSteerActive = true;
            } else if (bSteerRight && !bSteerLeft) {
                data.nIndicatorState = eIndicatorState::RightOn;
                data.bWasAutoSteerActive = true;
            } else if (data.bWasAutoSteerActive) {
                data.nIndicatorState = eIndicatorState::Off;
                data.bWasAutoSteerActive = false;
            }
        }
    } else if (pVeh->m_pDriver && !bSAMP) {
        data.nIndicatorState = eIndicatorState::Off;
        CVector2D prevPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nPreviousPathNodeInfo);
        CVector2D currPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nCurrentPathNodeInfo);
        CVector2D nextPoint = GetCarPathLinkPosition(pVeh->m_autoPilot.m_nNextPathNodeInfo);

        float angle = Util::NormalizeAngle(CGeneral::GetATanOfXY(nextPoint.x - currPoint.x, nextPoint.y - currPoint.y) * 57.295776f - 
                                           CGeneral::GetATanOfXY(currPoint.x - prevPoint.x, currPoint.y - prevPoint.y) * 57.295776f);

        if (angle >= 30.0f && angle < 180.0f) {
            data.nIndicatorState = eIndicatorState::LeftOn;
        } else if (angle <= 330.0f && angle > 180.0f) {
            data.nIndicatorState = eIndicatorState::RightOn;
        }

        if (data.nIndicatorState == eIndicatorState::Off) {
            if (pVeh->m_autoPilot.m_nCurrentLane == 0 && pVeh->m_autoPilot.m_nNextLane == 1) {
                data.nIndicatorState = eIndicatorState::RightOn;
            } else if (pVeh->m_autoPilot.m_nCurrentLane == 1 && pVeh->m_autoPilot.m_nNextLane == 0) {
                data.nIndicatorState = eIndicatorState::LeftOn;
            }
        }
    }
}

void IndicatorComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    if (!LightsConfig::Get().gbGlobalIndicatorLights && !LightManager::IsMaterialAvailable(pControlVeh, INDICATOR_LIGHTS_TYPE)) {
        return;
    }

    if (!BlinkerState::Get().bIndicatorsDelay || data.nIndicatorState == eIndicatorState::Off) return;

    auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
    bool isLeftFrontOk = damage.isFrontLeftOk;
    bool isRightFrontOk = damage.isFrontRightOk;
    bool isLeftRearOk = damage.isRearLeftOk;
    bool isRightRearOk = damage.isRearRightOk;
    bool isLeftMiddleOk = damage.isMiddleLeftOk;
    bool isRightMiddleOk = damage.isMiddleRightOk;

    // Global turn lights activation check
    if (LightsConfig::Get().gbGlobalIndicatorLights && !LightManager::IsMaterialAvailable(pControlVeh, INDICATOR_LIGHTS_TYPE) && !LightManager::IsMaterialAvailable(pControlVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight})) {
        if ((pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD) &&
            (pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_AUTOMOBILE || pControlVeh->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE) &&
            pControlVeh->bEngineOn && pControlVeh->m_fHealth > 0 && !pControlVeh->bIsDrowning && !pControlVeh->m_pAttachedTo) {
            data.bUsingGlobalIndicators = true;
        }
    } else {
        bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
        std::string shdwName = (isBike ? "taillight_bike" : "taillight");
        float shdwSz = 2.0f;

        if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::LeftOn) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::IndicatorLightLeftFront, true, "indicator", 1.0f, false, isLeftFrontOk);
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::IndicatorLightLeftMiddle, true, "indicator", 1.0f, false, isLeftMiddleOk);
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::IndicatorLightLeftRear, true, "indicator", 1.0f, false, isLeftRearOk);
            if (isLeftRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::STTLightLeft, true, shdwName, shdwSz, true, isLeftRearOk);
            }
        }

        if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::RightOn) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::IndicatorLightRightFront, true, "indicator", 1.0f, false, isRightFrontOk);
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::IndicatorLightRightMiddle, true, "indicator", 1.0f, false, isRightMiddleOk);
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::IndicatorLightRightRear, true, "indicator", 1.0f, false, isRightRearOk);
            if (isRightRearOk) {
                LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::STTLightRight, true, shdwName, shdwSz, true, isRightRearOk);
            }
        }
    }

    if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::LeftOn) {
        if (isLeftRearOk) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::NABrakeLightLeft, true, "indicator", 1.0f, false, isLeftRearOk);
        }
    }

    if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::RightOn) {
        if (isRightRearOk) {
            LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::NABrakeLightRight, true, "indicator", 1.0f, false, isRightRearOk);
        }
    }
}

void IndicatorComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    if (data.nIndicatorState != eIndicatorState::Off) {
        float indRadius = 1.40f;

        if (BlinkerState::Get().bIndicatorsDelay) {
            auto renderIndPointLight = [&](eMaterialType type, bool isDamaged) {
                if (isDamaged || !LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) return;
                for (auto e : data.dummies[type]) {
                    e->Update();
                    RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, indRadius, true);
                }
            };

            if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::LeftOn) {
                renderIndPointLight(eMaterialType::IndicatorLightLeftFront, Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT) || Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT));
                renderIndPointLight(eMaterialType::IndicatorLightLeftRear, Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_LEFT));
                renderIndPointLight(eMaterialType::IndicatorLightLeftMiddle, Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT));
                renderIndPointLight(eMaterialType::NABrakeLightLeft, Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_LEFT));
            }

            if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::RightOn) {
                renderIndPointLight(eMaterialType::IndicatorLightRightFront, Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT) || Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT));
                renderIndPointLight(eMaterialType::IndicatorLightRightRear, Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_RIGHT));
                renderIndPointLight(eMaterialType::IndicatorLightRightMiddle, Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT));
                renderIndPointLight(eMaterialType::NABrakeLightRight, Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_RIGHT));
            }
        }
        else if (data.bUsingGlobalIndicators && !LightManager::IsMaterialAvailable(pVeh, INDICATOR_LIGHTS_TYPE)) {
            auto renderGlobalIndPointLight = [&](eMaterialType type, bool isDamaged) {
                if (isDamaged || !LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) return;
                for (auto e : data.dummies[type]) {
                    e->Update();
                    RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, indRadius, true);
                }
            };

            if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::LeftOn) {
                renderGlobalIndPointLight(eMaterialType::TailLightLeft, Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_LEFT));
            }
            if (data.nIndicatorState == eIndicatorState::BothOn || data.nIndicatorState == eIndicatorState::RightOn) {
                renderGlobalIndPointLight(eMaterialType::TailLightRight, Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_RIGHT));
            }
        }
    }
}

