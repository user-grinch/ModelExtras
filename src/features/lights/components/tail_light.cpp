#include "pch.h"
#include "tail_light.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/render.h"
#include "../damage.h"
#include "defines.h"

void TailLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_TAILLIGHT_LEFT.ToInt()] = eMaterialType::TailLightLeft;
    matMap[VEHCOL_TAILLIGHT_RIGHT.ToInt()] = eMaterialType::TailLightRight;
}

eMaterialType TailLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_TAILLIGHT_LEFT) return eMaterialType::TailLightLeft;
    if (matCol == VEHCOL_TAILLIGHT_RIGHT) return eMaterialType::TailLightRight;
    return eMaterialType::UnknownMaterial;
}

bool TailLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name == "taillights" || name == "taillights2" || name == "taillight") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.corona.size = LightsConfig::Get().gfTailLightCoronaSize;
        c.corona.color = {250, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightCoronaIntensity)};
        c.shadow.color = {250, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfTailLightShadowSizeMul;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "taillights2";

        bool isBike = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE);

        if (isBike) {
            c.mirroredX = false;
            c.lightType = eMaterialType::TailLightRight;
            data.dummies[c.lightType].push_back(VehicleDummy(c));
        } else {
            bool dummyIsLeft = (c.frame->modelling.pos.x < 0.0f);
            c.mirroredX = dummyIsLeft;
            c.lightType = eMaterialType::TailLightRight;
            data.dummies[c.lightType].push_back(VehicleDummy(c));

            c.mirroredX = !dummyIsLeft;
            c.lightType = eMaterialType::TailLightLeft;
            data.dummies[c.lightType].push_back(VehicleDummy(c));
        }
        return true;
    }

    if (name == "taillight_l" || name == "taillight_r" || name == "taillights_l" || name == "taillights_r") {
        if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
            return false;
        }
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        bool isLeft = (name == "taillight_l" || name == "taillights_l");
        c.lightType = isLeft ? eMaterialType::TailLightLeft : eMaterialType::TailLightRight;
        c.corona.size = LightsConfig::Get().gfTailLightCoronaSize;
        c.corona.color = {250, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightCoronaIntensity)};
        c.shadow.color = {250, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfTailLightShadowSizeMul;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = true;
        c.mirroredX = false;
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }

    return false;
}

void TailLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isBike = CModelInfo::IsBikeModel(pControlVeh->m_nModelIndex);
    std::string shdwName = (isBike ? "taillight_bike" : "taillight");
    float shdwSz = 1.6f;

    if (pControlVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pControlVeh->m_nVehicleSubClass == VEHICLE_MTRUCK
        || pControlVeh->m_nVehicleSubClass == VEHICLE_QUAD || pControlVeh->m_nVehicleSubClass == VEHICLE_BIKE
        || pControlVeh->m_nVehicleSubClass == VEHICLE_TRAILER) 
    {
        auto damage = LightDamageState::Get(pControlVeh);
        bool isLeftRearOk = damage.isRearLeftOk;
        bool isRightRearOk = damage.isRearRightOk;

        bool indicatorOn = data.bUsingGlobalIndicators && data.nIndicatorState != eIndicatorState::Off;
        bool isAlarmActive = pControlVeh->m_nAlarmState != 0 && pControlVeh->m_nAlarmState != 0xFFFF;
        bool isAlarmLightOn = isAlarmActive && ((pControlVeh->m_nAlarmState & 0x100) != 0);

        bool tailLightFlag = (CarUtil::AreLightsOn(pControlVeh) || isAlarmLightOn);
        if (isAlarmActive && !isAlarmLightOn) {
            tailLightFlag = false;
        }

        pControlVeh->m_renderLights.m_bLeftRear = isLeftRearOk && tailLightFlag;
        pControlVeh->m_renderLights.m_bRightRear = isRightRearOk && tailLightFlag;

        if (pControlVeh != pTowedVeh) {
            auto towedDamage = LightDamageState::Get(pTowedVeh);
            pTowedVeh->m_renderLights.m_bLeftRear = towedDamage.isRearLeftOk && tailLightFlag;
            pTowedVeh->m_renderLights.m_bRightRear = towedDamage.isRearRightOk && tailLightFlag;
        }

        auto RenderTailForVeh = [&](CVehicle* pVeh, VehLightData& vData, bool leftOk, bool rightOk) {
            bool sttInstalled = LightManager::IsMaterialAvailable(pVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
            if (sttInstalled) return;

            bool hasDedicatedBrake = LightManager::IsMaterialAvailable(pVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight}) ||
                                     LightManager::IsDummyAvailable(vData, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
            bool isBraking = LightManager::IsBraking(pControlVeh);
            bool tailHighlight = !hasDedicatedBrake && isBraking;

            auto tailLightsRender = [&](bool lOk, bool rOk) {
                if (LightManager::IsMaterialAvailable(pVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) || LightManager::IsDummyAvailable(vData, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
                    LightManager::RenderLight(pVeh, vData, eMaterialType::TailLightLeft, true, shdwName, shdwSz, tailHighlight, lOk);
                    LightManager::RenderLight(pVeh, vData, eMaterialType::TailLightRight, true, shdwName, shdwSz, tailHighlight, rOk);
                } else if (LightManager::IsMaterialAvailable(pVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight}) || LightManager::IsDummyAvailable(vData, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
                    LightManager::RenderLight(pVeh, vData, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz, false, lOk);
                    LightManager::RenderLight(pVeh, vData, eMaterialType::BrakeLightRight, true, shdwName, shdwSz, false, rOk);
                }
            };

            if (indicatorOn) {
                if (data.nIndicatorState == eIndicatorState::BothOn) {
                    tailLightsRender(leftOk && !BlinkerState::Get().bIndicatorsDelay, rightOk && !BlinkerState::Get().bIndicatorsDelay);
                } else if (data.nIndicatorState == eIndicatorState::LeftOn) {
                    tailLightsRender(leftOk && !BlinkerState::Get().bIndicatorsDelay, rightOk && tailLightFlag);
                } else if (data.nIndicatorState == eIndicatorState::RightOn) {
                    tailLightsRender(leftOk && tailLightFlag, rightOk && !BlinkerState::Get().bIndicatorsDelay);
                }
            } else {
                tailLightsRender(leftOk && tailLightFlag, rightOk && tailLightFlag);
            }
        };

        if (tailLightFlag || indicatorOn) {
            RenderTailForVeh(pControlVeh, data, isLeftRearOk, isRightRearOk);
            if (pControlVeh != pTowedVeh) {
                auto towedDamage = LightDamageState::Get(pTowedVeh);
                RenderTailForVeh(pTowedVeh, LightManager::m_VehData.Get(pTowedVeh), towedDamage.isRearLeftOk, towedDamage.isRearRightOk);
            }
        }
    }
}

void TailLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    bool isAlarmActive = pVeh->m_nAlarmState != 0 && pVeh->m_nAlarmState != 0xFFFF;
    bool isAlarmLightOn = isAlarmActive && ((pVeh->m_nAlarmState & 0x100) != 0);

    bool isHeadlightsOn = (CarUtil::AreLightsOn(pVeh) || isAlarmLightOn);
    if (isAlarmActive && !isAlarmLightOn) {
        isHeadlightsOn = false;
    }
    bool isBraking = LightManager::IsBraking(pVeh);
    bool isBike = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE || CModelInfo::IsBikeModel(pVeh->m_nModelIndex));
    bool hasDedicatedBrakeDummy = LightManager::IsDummyAvailable(data, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight});

    // 1. Taillights
    if (isHeadlightsOn || (isBraking && !hasDedicatedBrakeDummy)) {
        // Proper Shaders uses physically-based inverse square falloff, so scale down radius to prevent excessive red bleed
        float tailRadius = isBike ? (gbProperShadersDetected ? 1.2f : 1.5f) : (gbProperShadersDetected ? 2.5f : 3.5f);
        constexpr float tailPointLightMul = 0.40f;

        auto renderSingleTailLight = [&](eMaterialType actualType, eLights lightEnum, ePanels wingEnum, bool isLeft) {
            if (!LightManager::IsDummyAvailable(data, actualType) || !data.bLightStates[actualType]) {
                return;
            }
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) {
                return;
            }

            if (data.bUsingGlobalIndicators && !LightManager::IsMaterialAvailable(pVeh, INDICATOR_LIGHTS_TYPE)) {
                if (data.nIndicatorState == eIndicatorState::BothOn) return;
                if (data.nIndicatorState == eIndicatorState::LeftOn && isLeft) return;
                if (data.nIndicatorState == eIndicatorState::RightOn && !isLeft) return;
            }

            for (auto& e : data.dummies[actualType]) {
                e->Update();
                CRGBA baseCol = e->Get().corona.color;
                CRGBA tailColor = (isBraking && !hasDedicatedBrakeDummy)
                    ? CRGBA(255, 20, 20, 255)
                    : CRGBA(static_cast<unsigned char>(baseCol.r * tailPointLightMul),
                            static_cast<unsigned char>(baseCol.g * tailPointLightMul),
                            static_cast<unsigned char>(baseCol.b * tailPointLightMul),
                            baseCol.a);
                RenderUtil::RegisterPointLight(&e->Get(), tailColor, tailRadius, true);
            }
        };

        // If a motorcycle has only a single taillight dummy, render it once to avoid duplicate point lights
        bool isSingleBikeTail = isBike && (!LightManager::IsDummyAvailable(data, eMaterialType::TailLightLeft) || !LightManager::IsDummyAvailable(data, eMaterialType::TailLightRight));
        if (isSingleBikeTail) {
            eMaterialType singleType = LightManager::IsDummyAvailable(data, eMaterialType::TailLightRight) ? eMaterialType::TailLightRight : eMaterialType::TailLightLeft;
            bool isLeft = (singleType == eMaterialType::TailLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
            renderSingleTailLight(singleType, lightEnum, wingEnum, isLeft);
        } else {
            renderSingleTailLight(eMaterialType::TailLightLeft, eLights::LIGHT_REAR_LEFT, ePanels::WING_REAR_LEFT, true);
            renderSingleTailLight(eMaterialType::TailLightRight, eLights::LIGHT_REAR_RIGHT, ePanels::WING_REAR_RIGHT, false);
        }
    }

    // 2. Dedicated Brake Lights
    if (isBraking) {
        for (eMaterialType t : {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight}) {
            if (!LightManager::IsDummyAvailable(data, t) || !data.bLightStates[t]) continue;
            bool isLeft = (t == eMaterialType::BrakeLightLeft || t == eMaterialType::STTLightLeft || t == eMaterialType::NABrakeLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) continue;

            for (auto& e : data.dummies[t]) {
                e->Update();
                RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, isBike ? 1.5f : 3.5f, true);
            }
        }
    }
}
