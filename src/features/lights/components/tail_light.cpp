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
    if (name == "taillights" || name == "taillights2") {
        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Rear;
        c.lightType = eMaterialType::TailLightRight;
        c.corona.size = LightsConfig::Get().gfTailLightCoronaSize;
        c.corona.color = {250, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightCoronaIntensity)};
        c.shadow.color = {250, 0, 0, static_cast<unsigned char>(LightsConfig::Get().gTailLightShadowIntensity)};
        c.shadow.size = LightsConfig::Get().gfTailLightShadowSize;
        c.corona.lightingType = eLightingMode::Directional;
        c.shadow.render = name != "taillights2";
        c.mirroredX = false;
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        
        if (pVeh->m_nVehicleSubClass != VEHICLE_BIKE || std::abs(c.frame->modelling.pos.x) > 0.05f) {
            c.mirroredX = true;
            c.lightType = eMaterialType::TailLightLeft;
            data.dummies[c.lightType].push_back(VehicleDummy(c));
        }
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
        auto damage = LightDamageState::Get(pControlVeh, pTowedVeh);
        bool isLeftRearOk = damage.isRearLeftOk;
        bool isRightRearOk = damage.isRearRightOk;

        bool indicatorOn = data.bUsingGlobalIndicators && data.nIndicatorState != eIndicatorState::Off;
        bool tailLightFlag = (Util::IsNightTime() || pControlVeh->bLightsOn || CarUtil::IsLightsForcedOn(pControlVeh)) && !CarUtil::IsLightsForcedOff(pControlVeh);
        bool sttInstalled = LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::STTLightLeft, eMaterialType::STTLightRight});

        if ((tailLightFlag || indicatorOn) && !sttInstalled) {
            bool hasDedicatedBrake = LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight}) ||
                                     LightManager::IsDummyAvailable(data, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
            bool isBraking = (pControlVeh->m_fBreakPedal > 0.05f) && (pControlVeh->m_pDriver != nullptr);
            bool tailHighlight = !hasDedicatedBrake && isBraking;

            auto tailLightsRender = [&](bool leftOk, bool rightOk) {
                if (LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) || LightManager::IsDummyAvailable(data, {eMaterialType::TailLightLeft, eMaterialType::TailLightRight})) {
                    if (leftOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::TailLightLeft, true, shdwName, shdwSz, tailHighlight, leftOk);
                    }
                    if (rightOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::TailLightRight, true, shdwName, shdwSz, tailHighlight, rightOk);
                    }
                } else if (LightManager::IsMaterialAvailable(pTowedVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight}) || LightManager::IsDummyAvailable(data, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight})) {
                    if (leftOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::BrakeLightLeft, true, shdwName, shdwSz, false, leftOk);
                    }
                    if (rightOk) {
                        LightManager::RenderLights(pControlVeh, pTowedVeh, data, eMaterialType::BrakeLightRight, true, shdwName, shdwSz, false, rightOk);
                    }
                }
            };

            if (indicatorOn) {
                if (data.nIndicatorState == eIndicatorState::BothOn) {
                    tailLightsRender(isLeftRearOk && !BlinkerState::Get().bIndicatorsDelay, isRightRearOk && !BlinkerState::Get().bIndicatorsDelay);
                }

                if (data.nIndicatorState == eIndicatorState::LeftOn) {
                    tailLightsRender(isLeftRearOk && !BlinkerState::Get().bIndicatorsDelay, isRightRearOk && tailLightFlag);
                }

                if (data.nIndicatorState == eIndicatorState::RightOn) {
                    tailLightsRender(isLeftRearOk && tailLightFlag, isRightRearOk && !BlinkerState::Get().bIndicatorsDelay);
                }
            } else {
                tailLightsRender(isLeftRearOk, isRightRearOk);
            }
        }
    }
}

void TailLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    bool isHeadlightsOn = (pVeh->bLightsOn || CarUtil::IsLightsForcedOn(pVeh) || (Util::IsNightTime() && !Util::IsEngineOff(pVeh)) || (pVeh->m_nVehicleSubClass == VEHICLE_BIKE && !Util::IsEngineOff(pVeh))) && !CarUtil::IsLightsForcedOff(pVeh);
    bool isBraking = (pVeh->m_fBreakPedal > 0.05f) && (pVeh->m_pDriver != nullptr);
    bool isBike = CModelInfo::IsBikeModel(pVeh->m_nModelIndex);
    bool hasDedicatedBrakeDummy = LightManager::IsDummyAvailable(data, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight});

    // 1. Taillights
    if (isHeadlightsOn || (isBraking && !hasDedicatedBrakeDummy)) {
        float tailRadius = 3.5f;
        constexpr float tailPointLightMul = 0.40f;

        for (eMaterialType t : {eMaterialType::TailLightLeft, eMaterialType::TailLightRight}) {
            eMaterialType actualType = t;
            if (!LightManager::IsDummyAvailable(data, t) && isBike) {
                if (LightManager::IsDummyAvailable(data, eMaterialType::TailLightRight))
                    actualType = eMaterialType::TailLightRight;
                else if (LightManager::IsDummyAvailable(data, eMaterialType::TailLightLeft))
                    actualType = eMaterialType::TailLightLeft;
            }

            if (!LightManager::IsDummyAvailable(data, actualType) || !data.bLightStates[actualType]) {
                continue;
            }

            bool isLeft = (t == eMaterialType::TailLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_REAR_LEFT : eLights::LIGHT_REAR_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_REAR_LEFT : ePanels::WING_REAR_RIGHT;
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) {
                continue;
            }

            if (data.bUsingGlobalIndicators && !LightManager::IsMaterialAvailable(pVeh, INDICATOR_LIGHTS_TYPE)) {
                if (data.nIndicatorState == eIndicatorState::BothOn) continue;
                if (data.nIndicatorState == eIndicatorState::LeftOn && isLeft) continue;
                if (data.nIndicatorState == eIndicatorState::RightOn && !isLeft) continue;
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
                RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 3.5f, true);
            }
        }
    }
}
