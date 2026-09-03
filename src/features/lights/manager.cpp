#include "pch.h"
#include "manager.h"
#include "utils/modelinfomgr.h"
#include "utils/render.h"
#include "utils/util.h"
#include "utils/car.h"
#include "utils/datamgr.h"
#include "defines.h"
#include "components/headlight.h"
#include "components/indicator.h"
#include "components/reverse_light.h"
#include "components/brake_light.h"
#include "components/tail_light.h"
#include "components/stt_light.h"
#include "components/nabrake_light.h"
#include "components/fog_light.h"
#include "components/strobe_light.h"
#include "components/drl_light.h"
#include "components/side_light.h"
#include "components/spot_light.h"

void LightManager::Init() {
    m_Components.clear();
    m_MaterialMap.clear();

    m_Components.push_back(std::make_unique<HeadlightComponent>());
    m_Components.push_back(std::make_unique<IndicatorComponent>());
    m_Components.push_back(std::make_unique<ReverseLightComponent>());
    m_Components.push_back(std::make_unique<BrakeLightComponent>());
    m_Components.push_back(std::make_unique<TailLightComponent>());
    m_Components.push_back(std::make_unique<STTLightComponent>());
    m_Components.push_back(std::make_unique<NABrakeLightComponent>());
    m_Components.push_back(std::make_unique<FogLightComponent>());
    m_Components.push_back(std::make_unique<StrobeLightComponent>());
    m_Components.push_back(std::make_unique<DRLLightComponent>());
    m_Components.push_back(std::make_unique<SideLightComponent>());
    m_Components.push_back(std::make_unique<SpotLightComponent>());

    for (const auto& comp : m_Components) {
        comp->RegisterMaterials(m_MaterialMap);
    }

    ModelInfoMgr::RegisterMaterialColProvider([](CVehicle* pVeh, RpMaterial* pMat, eMaterialType type) -> MatStateColor {
        if (type == eMaterialType::HeadLightLeft || type == eMaterialType::HeadLightRight) {
            bool longLights = pVeh && m_VehData.Get(pVeh).bLongLightsOn;
            if (longLights) {
                return { CRGBA(255, 255, 255, 255), DEFAULT_MAT_COL };
            } else {
                return { CRGBA(100, 100, 100, 255), DEFAULT_MAT_COL };
            }
        }
        if (type == eMaterialType::TailLightLeft || type == eMaterialType::TailLightRight) {
            if (pVeh) {
                bool hasDedicatedBrake = LightManager::IsMaterialAvailable(pVeh, {eMaterialType::BrakeLightLeft, eMaterialType::BrakeLightRight, eMaterialType::NABrakeLightLeft, eMaterialType::NABrakeLightRight, eMaterialType::STTLightLeft, eMaterialType::STTLightRight});
                if (!hasDedicatedBrake) {
                    bool isBraking = (pVeh->m_fBreakPedal > 0.05f) && (pVeh->m_pDriver != nullptr);
                    if (isBraking) {
                        return { CRGBA(255, 255, 255, 255), DEFAULT_MAT_COL };
                    } else {
                        return { CRGBA(180, 180, 180, 255), DEFAULT_MAT_COL };
                    }
                }
            }
        }
        return { DEFAULT_MAT_COL, DEFAULT_MAT_COL };
    });
}

DummyConfig LightManager::CreateBaseConfig(CVehicle* pVeh, RwFrame* pFrame) {
    DummyConfig c;
    c.frame = pFrame;
    c.position = pFrame->modelling.pos;
    c.pVeh = pVeh;
    c.corona.size = LightsConfig::Get().gfGlobalCoronaSize;
    c.corona.color = {255, 255, 255, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
    c.corona.lightingType = eLightingMode::NonDirectional;
    return c;
}

eMaterialType LightManager::GetMatType(RpMaterial* pMat) {
    if (Util::IsAntiPatternLightMaterial(pMat)) {
        return eMaterialType::UnknownMaterial;
    }

    CRGBA matCol = *reinterpret_cast<CRGBA*>(RpMaterialGetColor(pMat));
    matCol.a = 255;

    auto it = m_MaterialMap.find(matCol.ToInt());
    if (it != m_MaterialMap.end()) {
        return it->second;
    }

    return eMaterialType::UnknownMaterial;
}

void LightManager::RegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name) {
    if (pFrame && !rwLinkListEmpty(&pFrame->objectList)) {
        return;
    }
    VehLightData& data = m_VehData.Get(pVeh);

    for (const auto& comp : m_Components) {
        if (comp->TryRegisterDummy(pVeh, pFrame, name, data)) return;
    }
}

void LightManager::Process(CVehicle* pVeh) {
    if (!pVeh) return;

    if (!CarUtil::AreHeadlightsActive(pVeh) || !CarUtil::AreHeadlightsPopUpOpen(pVeh)) {
        pVeh->m_renderLights.m_bLeftFront = false;
        pVeh->m_renderLights.m_bRightFront = false;
    }

    VehLightData& data = m_VehData.Get(pVeh);
    for (const auto& comp : m_Components) {
        comp->Process(pVeh, data);
    }
}

void LightManager::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh) {
    VehLightData& data = m_VehData.Get(pControlVeh);
    eIndicatorState indState = data.nIndicatorState;

    // Fix for UIF SAMP server https://github.com/user-grinch/ModelExtras/issues/112
    // Don't clear light state when lights are forced on/already on via SAMP
    if (pControlVeh->m_fHealth <= 0.0f || ((Util::IsEngineOff(pControlVeh) && indState == eIndicatorState::Off) && !CarUtil::IsLightsForcedOn(pControlVeh) && !pControlVeh->bLightsOn) || CarUtil::IsLightsForcedOff(pControlVeh)) {
        pControlVeh->bLightsOn = false;
        pControlVeh->m_renderLights.m_bLeftFront = false;
        pControlVeh->m_renderLights.m_bRightFront = false;
        pControlVeh->m_renderLights.m_bLeftRear = false;
        pControlVeh->m_renderLights.m_bRightRear = false;
    }

    // Fix for park car alarm lights
    // Allow through if lights or indicators are explicitly on
    if (pControlVeh->m_fHealth <= 0.0f || ((Util::IsEngineOff(pControlVeh) && indState == eIndicatorState::Off) && !CarUtil::IsLightsForcedOn(pControlVeh) && !pControlVeh->bLightsOn)) {
        return;
    }

    bool isHeadlightsActive = CarUtil::AreHeadlightsActive(pControlVeh);
    bool bPopUpOpen = CarUtil::AreHeadlightsPopUpOpen(pControlVeh);
    static bool bHeadLightBeams = gConfig.ReadBoolean("LIGHTS", "HeadLightBeams", gConfig.ReadBoolean("TWEAKS", "HeadLightBeams", true));
    bool isLeftFrontDamaged = Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT) || Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_LEFT);
    bool isRightFrontDamaged = Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT) || Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_RIGHT);
    pControlVeh->m_renderLights.m_bLeftFront = isHeadlightsActive && bPopUpOpen && !isLeftFrontDamaged && bHeadLightBeams;
    pControlVeh->m_renderLights.m_bRightFront = isHeadlightsActive && bPopUpOpen && !isRightFrontDamaged && bHeadLightBeams;

    for (const auto& comp : m_Components) {
        comp->Render(pControlVeh, pTowedVeh, data);
    }
}

void LightManager::EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul) {
    if (LightsConfig::Get().gbLightCoronasFeature) {
        const DummyConfig &c = dummy->GetRef();
        if (c.corona.lightingType == eLightingMode::NonDirectional) {
            RenderUtil::RegisterCorona(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + id, c.position, c.corona.color, c.corona.size * szMul);
        } else {
            RenderUtil::RegisterCoronaDirectional(&dummy->Get(), c.rotation.angle, 180.0f, szMul, c.corona.lightingType == eLightingMode::Inversed, false);
        }
    }
}

void LightManager::RenderLight(CVehicle* pVeh, VehLightData& data, eMaterialType type, bool isOn, const std::string& texture, float sz, bool highlight, bool isDummyOk, bool materialsOnly) {
    if (!isOn || !data.bLightStates[type]) return;

    int id = static_cast<int>(type) * 1000;
    bool hasActiveDummy = false;
    bool isAvailable = IsDummyAvailable(data, type);

    if (isAvailable) {
        for (auto* dummy : data.dummies[type]) {
            const DummyConfig& c = dummy->GetRef();
            dummy->Update();
            RwFrame *parent = RwFrameGetParent(dummy->Get().frame);
            bool isBike = pVeh->m_nVehicleSubClass == VEHICLE_BIKE;
            bool isDamaged = Util::IsFrameDamaged(pVeh, parent) || !FrameUtil::IsOkAtomicVisible(parent);
            bool atomicCheck = !isBike && pVeh->GetIsOnScreen() && type != eMaterialType::HeadLightLeft && type != eMaterialType::HeadLightRight && isDamaged;

            if (atomicCheck || (c.dummyPos == eDummyPos::Rear && pVeh->m_pTrailer) || !isDummyOk) {
                continue;
            }

            hasActiveDummy = true;

            if (type == eMaterialType::StrobeLight) {
                size_t timer = CTimer::m_snTimeInMilliseconds;
                if (timer - c.strobe.timer > c.strobe.delay) {
                    dummy->Get().strobe.enabled = !c.strobe.enabled;
                    dummy->Get().strobe.timer = timer;
                }

                if (c.strobe.enabled) {
                    ModelInfoMgr::EnableStrobeMaterial(pVeh, c.dummyIdx);
                } else {
                    continue;
                }
            }

            if (materialsOnly) {
                continue;
            }

            float szMul = 1.0f;
            if (highlight) {
                szMul = (type == eMaterialType::TailLightLeft || type == eMaterialType::TailLightRight) ? 1.50f : 3.00f;
            }
            EnableDummy((int)pVeh + 42 + id++, dummy, pVeh, szMul);

            // Skip front shadows on bike wheelie
            if (c.dummyPos == eDummyPos::Front && Util::IsVehicleDoingWheelie(pVeh)) {
                continue;
            }

            if (c.shadow.render) {
                std::string tex = (c.shadow.texture == "") ? texture : c.shadow.texture;
                if (!tex.empty()) {
                    RenderUtil::RegisterShadowDirectional(&dummy->Get(), tex, sz * c.shadow.size);
                }
            }
        }
    }

    if (!isAvailable || hasActiveDummy) {
        ModelInfoMgr::EnableMaterial(pVeh, type);
    }
}

void LightManager::RenderLights(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data, eMaterialType type, bool isOn, const std::string& texture, float sz, bool highlight, bool isDummyOk, bool materialsOnly) {
    if (data.bLightStates[type]) {
        RenderLight(pControlVeh, data, type, isOn, texture, sz, highlight, isDummyOk, materialsOnly);
    }

    if (pControlVeh != pTowedVeh && m_VehData.Get(pTowedVeh).bLightStates[type]) {
        RenderLight(pTowedVeh, m_VehData.Get(pTowedVeh), type, isOn, texture, sz, highlight, isDummyOk, materialsOnly);
    }
}

bool LightManager::IsDummyAvailable(VehLightData& data, eMaterialType type) {
    if (type < 0 || type >= eMaterialType::TotalMaterial) return false;
    return !data.dummies[type].empty();
}

bool LightManager::IsDummyAvailable(VehLightData& data, std::initializer_list<eMaterialType> types) {
    for (eMaterialType type : types) {
        if (IsDummyAvailable(data, type)) return true;
    }
    return false;
}

bool LightManager::IsMaterialAvailable(CVehicle* pVeh, eMaterialType type) {
    return ModelInfoMgr::IsMaterialAvailable(pVeh, type);
}

bool LightManager::IsMaterialAvailable(CVehicle* pVeh, std::initializer_list<eMaterialType> types) {
    for (eMaterialType type : types) {
        if (IsMaterialAvailable(pVeh, type)) return true;
    }
    return false;
}

void LightManager::ProcessPointLights(CVehicle *pVeh) {
    if (!LightsConfig::Get().gbLightPointLights || !pVeh || pVeh->m_fHealth <= 0.0f || pVeh->m_nVehicleSubClass == VEHICLE_BMX || pVeh->m_nVehicleSubClass == VEHICLE_BOAT || pVeh->m_nVehicleSubClass == VEHICLE_TRAILER) {
        return;
    }

    if (CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition()) > 75.0f) {
        return;
    }

    VehLightData &data = m_VehData.Get(pVeh);
    for (const auto& comp : m_Components) {
        comp->ProcessPointLights(pVeh, data);
    }
}

void LightManager::Reload(CVehicle* pVeh) {
    LightsConfig::Get().InitConfig();
    if (pVeh) {
        m_VehData.Get(pVeh).ClearDummies();
        DataMgr::Reload(pVeh->m_nModelIndex);
    }
}
