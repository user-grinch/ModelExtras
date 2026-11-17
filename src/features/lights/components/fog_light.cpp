#include "pch.h"
#include "fog_light.h"
#include "utils/audiomgr.h"
#include "utils/render.h"
#include "utils/datamgr.h"
#include "defines.h"
#include "../damage.h"

void FogLightComponent::RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {
    matMap[VEHCOL_FOGLIGHT_LEFT.ToInt()] = eMaterialType::FogLightLeft;
    matMap[VEHCOL_FOGLIGHT_RIGHT.ToInt()] = eMaterialType::FogLightRight;
}

eMaterialType FogLightComponent::GetMatType(CRGBA matCol) {
    if (matCol == VEHCOL_FOGLIGHT_LEFT) return eMaterialType::FogLightLeft;
    if (matCol == VEHCOL_FOGLIGHT_RIGHT) return eMaterialType::FogLightRight;
    return eMaterialType::UnknownMaterial;
}

bool FogLightComponent::TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) {
    if (name.starts_with("fogl") || name.starts_with("foglight")) {
        bool isLeft = name.contains("_l") || name.contains("_l_");
        bool isRight = name.contains("_r") || name.contains("_r_");
        if (!isLeft && !isRight) return false;

        DummyConfig c = LightManager::CreateBaseConfig(pVeh, pFrame);
        c.dummyPos = eDummyPos::Front;
        c.lightType = isLeft ? eMaterialType::FogLightLeft : eMaterialType::FogLightRight;
        c.shadow.render = false;
        c.shadow.texture = "foglight";
        c.shadow.size = 1.0f;
        c.corona.color = c.shadow.color = {255, 242, 225, static_cast<unsigned char>(LightsConfig::Get().gGlobalCoronaIntensity)};
        c.corona.lightingType = eLightingMode::Directional;
        data.dummies[c.lightType].push_back(VehicleDummy(c));
        return true;
    }
    return false;
}

void FogLightComponent::Process(CVehicle* pVeh, VehLightData& data) {
    CPed* pPlayer = FindPlayerPed();
    if (pPlayer && pVeh->IsDriver(pPlayer)) {
        bool isHeadlightsActive = CarUtil::AreLightsOn(pVeh);
        bool canToggleFogLight = !LightsConfig::Get().bFoglightTiedToHeadlight || isHeadlightsActive;

        bool hasFogSupport = LightManager::IsMaterialAvailable(pVeh, {eMaterialType::FogLightLeft, eMaterialType::FogLightRight, eMaterialType::FogLightLed})
                          || LightManager::IsDummyAvailable(data, {eMaterialType::FogLightLeft, eMaterialType::FogLightRight});

        if (!hasFogSupport && DataMgr::Has(pVeh->m_nModelIndex)) {
            const auto& j = DataMgr::Get(pVeh->m_nModelIndex);
            if (j.contains("lights")) {
                const auto& l = j["lights"];
                if (l.contains("foglights") || l.contains("foglight") || l.contains("fogl") ||
                    l.contains("foglight_l") || l.contains("foglight_r") || l.contains("fogl_l") || l.contains("fogl_r")) {
                    hasFogSupport = true;
                }
            }
        }

        if (InputMgr::IsKeyJustDown(LightsConfig::Get().nFogLightKey) && hasFogSupport && canToggleFogLight) {
            data.bFogLightsOn = !data.bFogLightsOn;
            AudioMgr::PlaySwitchSound();
        }
    }
}

void FogLightComponent::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {
    bool isHeadlightsActive = CarUtil::AreLightsOn(pControlVeh);
    bool isFoggy = Util::IsFoggy();
    bool shouldRenderFog = isFoggy || !LightsConfig::Get().bFoglightTiedToHeadlight || isHeadlightsActive;
    bool isFogLightOn = (data.bFogLightsOn || isFoggy) && (!LightsConfig::Get().bFoglightTiedToHeadlight || !CarUtil::IsLightsForcedOff(pControlVeh));

    if (!isFogLightOn || !shouldRenderFog) return;
    auto damage = LightDamageState::Get(pControlVeh);
    LightManager::RenderLight(pControlVeh, data, eMaterialType::FogLightLeft, true, "foglight", 3.0f, false, damage.isHeadlightLeftOk);
    LightManager::RenderLight(pControlVeh, data, eMaterialType::FogLightRight, true, "foglight", 3.0f, false, damage.isHeadlightRightOk);
}

void FogLightComponent::ProcessPointLights(CVehicle* pVeh, VehLightData& data) {
    bool isHeadlightsOn = CarUtil::AreLightsOn(pVeh);
    bool isFoggy = Util::IsFoggy();
    bool shouldRenderFog = isFoggy || !LightsConfig::Get().bFoglightTiedToHeadlight || isHeadlightsOn;
    bool isFogLightOn = (data.bFogLightsOn || isFoggy) && (!LightsConfig::Get().bFoglightTiedToHeadlight || !CarUtil::IsLightsForcedOff(pVeh));

    if (isFogLightOn && shouldRenderFog) {
        for (eMaterialType type : {eMaterialType::FogLightLeft, eMaterialType::FogLightRight}) {
            if (!LightManager::IsDummyAvailable(data, type) || !data.bLightStates[type]) continue;

            bool isLeft = (type == eMaterialType::FogLightLeft);
            eLights lightEnum = isLeft ? eLights::LIGHT_FRONT_LEFT : eLights::LIGHT_FRONT_RIGHT;
            ePanels wingEnum = isLeft ? ePanels::WING_FRONT_LEFT : ePanels::WING_FRONT_RIGHT;
            if (Util::IsLightDamaged(pVeh, lightEnum) || Util::IsPanelDamaged(pVeh, wingEnum)) {
                continue;
            }

            for (auto& e : data.dummies[type]) {
                e->Update();
                RenderUtil::RegisterPointLight(&e->Get(), e->Get().corona.color, 8.5f, true);
            }
        }
    }
}
