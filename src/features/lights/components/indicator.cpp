#include "pch.h"
#include "indicator.h"
#include "utils/util.h"
#include "utils/car.h"
#include <CPathFind.h>

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
        if (d) {
            DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
            bool isLeft = (d.value()[0] == 'L');
            c.corona.color = {255, 128, 0, 255};
            c.corona.lightingType = eLightingMode::Directional;
            switch (d.value()[1]) {
                case 'F': c.lightType = isLeft ? eMaterialType::IndicatorLightLeftFront : eMaterialType::IndicatorLightRightFront; c.dummyPos = eDummyPos::Front; break;
                case 'R': c.lightType = isLeft ? eMaterialType::IndicatorLightLeftRear : eMaterialType::IndicatorLightRightRear; c.dummyPos = eDummyPos::Rear; break;
                case 'M': c.lightType = isLeft ? eMaterialType::IndicatorLightLeftMiddle : eMaterialType::IndicatorLightRightMiddle; c.dummyPos = isLeft ? eDummyPos::Left : eDummyPos::Right; break;
            }
            data.dummies[c.lightType].push_back(new VehicleDummy(c));
            return true;
        }
    }
    return false;
}

static CVector2D GetCarPathLinkPosition(CCarPathLinkAddress &address) {
    if (address.m_nAreaId >= 0 && address.m_nCarPathLinkId >= 0 && ThePaths.m_pNaviNodes && ThePaths.m_pNaviNodes[address.m_nAreaId]) {
        return CVector2D(static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.x) / 8.0f,
                         static_cast<float>(ThePaths.m_pNaviNodes[address.m_nAreaId][address.m_nCarPathLinkId].m_vecPosn.y) / 8.0f);
    }
    return CVector2D(0.0f, 0.0f);
}

void IndicatorComponent::Process(CVehicle* pVeh, VehLightData& data) {
    static bool bSAMP = GetModuleHandle("SAMP.asi") || GetModuleHandle("SAMP.dll");

    if (pVeh->m_pDriver == FindPlayerPed()) {
        static uint32_t indicatorNoneKey = gConfig.ReadInteger("KEYS", "IndicatorLightNoneKey", VK_SHIFT);
        static uint32_t indicatorLeftKey = gConfig.ReadInteger("KEYS", "IndicatorLightLeftKey", VK_Z);
        static uint32_t indicatorRightKey = gConfig.ReadInteger("KEYS", "IndicatorLightRightKey", VK_C);
        static uint32_t indicatorBothKey = gConfig.ReadInteger("KEYS", "IndicatorLightBothKey", VK_X);

        if (KeyPressed(indicatorNoneKey)) {
            data.nIndicatorState = eIndicatorState::Off;
        } else if (KeyPressed(indicatorLeftKey)) {
            data.nIndicatorState = eIndicatorState::LeftOn;
        } else if (KeyPressed(indicatorRightKey)) {
            data.nIndicatorState = eIndicatorState::RightOn;
        } else if (KeyPressed(indicatorBothKey)) {
            data.nIndicatorState = eIndicatorState::BothOn;
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
    if (data.nIndicatorState == eIndicatorState::Off || !LightsGlobal::Get().bIndicatorsDelay) return;

    bool leftOn = (data.nIndicatorState == eIndicatorState::LeftOn || data.nIndicatorState == eIndicatorState::BothOn);
    bool rightOn = (data.nIndicatorState == eIndicatorState::RightOn || data.nIndicatorState == eIndicatorState::BothOn);

    bool isLeftFrontOk = !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT);
    bool isRightFrontOk = !Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT);
    bool isLeftRearOk = !Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT);
    bool isRightRearOk = !Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT);

    if (leftOn) {
        LightManager::RenderLight(pControlVeh, data, eMaterialType::IndicatorLightLeftFront, isLeftFrontOk, "indicator");
        LightManager::RenderLight(pControlVeh, data, eMaterialType::IndicatorLightLeftMiddle, true, "indicator");
        LightManager::RenderLight(pTowedVeh, data, eMaterialType::IndicatorLightLeftRear, isLeftRearOk, "indicator");
    }

    if (rightOn) {
        LightManager::RenderLight(pControlVeh, data, eMaterialType::IndicatorLightRightFront, isRightFrontOk, "indicator");
        LightManager::RenderLight(pControlVeh, data, eMaterialType::IndicatorLightRightMiddle, true, "indicator");
        LightManager::RenderLight(pTowedVeh, data, eMaterialType::IndicatorLightRightRear, isRightRearOk, "indicator");
    }
}

