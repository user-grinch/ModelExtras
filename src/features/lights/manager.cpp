#include "pch.h"
#include "manager.h"
#include "utils/modelinfomgr.h"
#include "utils/render.h"
#include "utils/util.h"
#include "defines.h"
#include "components/headlight.h"
#include "components/indicator.h"
#include "components/rear_lights.h"
#include "components/fog_light.h"
#include "components/strobe_light.h"
#include "components/auxiliary_light.h"
#include "components/spot_light.h"

void LightManager::Init() {}

DummyConfig LightManager::CreateBaseConfig(CVehicle* pVeh, RwFrame* pFrame) {
    DummyConfig c;
    c.pVeh = pVeh;
    c.frame = pFrame;
    c.corona.color = {255, 255, 255, static_cast<unsigned char>(LightsGlobal::Get().gGlobalCoronaIntensity)};
    return c;
}

eMaterialType LightManager::GetMatType(RpMaterial* pMat) {
    CRGBA matCol = *reinterpret_cast<CRGBA*>(RpMaterialGetColor(pMat));
    matCol.a = 255;

    eMaterialType type = eMaterialType::UnknownMaterial;
    
    type = HeadlightComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;
    
    type = IndicatorComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;

    type = RearLightsComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;

    type = FogLightComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;

    type = StrobeLightComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;

    type = AuxiliaryLightComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;

    type = SpotLightComponent::GetMatType(matCol);
    if (type != eMaterialType::UnknownMaterial) return type;

    return eMaterialType::UnknownMaterial;
}

void LightManager::RegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name) {
    VehLightData& data = m_VehData.Get(pVeh);

    if (HeadlightComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
    if (IndicatorComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
    if (RearLightsComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
    if (FogLightComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
    if (StrobeLightComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
    if (AuxiliaryLightComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
    if (SpotLightComponent::TryRegisterDummy(pVeh, pFrame, name, data)) return;
}

void LightManager::Process(CVehicle* pVeh) {
    if (!pVeh) return;

    static uint64_t lastFlashUpdate = 0;
    uint64_t now = CTimer::m_snTimeInMilliseconds;
    if (now - lastFlashUpdate > 500) {
        LightsGlobal::Get().bIndicatorsDelay = !LightsGlobal::Get().bIndicatorsDelay;
        lastFlashUpdate = now;
    }

    if (Util::IsEngineOff(pVeh)) return;

    VehLightData& data = m_VehData.Get(pVeh);
    HeadlightComponent::Process(pVeh, data);
    IndicatorComponent::Process(pVeh, data);
    RearLightsComponent::Process(pVeh, data);
    FogLightComponent::Process(pVeh, data);
    StrobeLightComponent::Process(pVeh, data);
    AuxiliaryLightComponent::Process(pVeh, data);
    SpotLightComponent::Process(pVeh, data);
}

void LightManager::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh) {
    if (Util::IsEngineOff(pControlVeh)) return;

    VehLightData& data = m_VehData.Get(pControlVeh);
    HeadlightComponent::Render(pControlVeh, pTowedVeh, data);
    IndicatorComponent::Render(pControlVeh, pTowedVeh, data);
    RearLightsComponent::Render(pControlVeh, pTowedVeh, data);
    FogLightComponent::Render(pControlVeh, pTowedVeh, data);
    StrobeLightComponent::Render(pControlVeh, pTowedVeh, data);
    AuxiliaryLightComponent::Render(pControlVeh, pTowedVeh, data);
    SpotLightComponent::Render(pControlVeh, pTowedVeh, data);
}

void LightManager::RenderLight(CVehicle* pVeh, VehLightData& data, eMaterialType type, bool isOn, const std::string& texture) {
    if (!isOn) return;

    if (IsDummyAvailable(data, type)) {
        for (auto* dummy : data.dummies[type]) {
            dummy->Update();
            const DummyConfig& c = dummy->GetRef();

            if (gConfig.ReadBoolean("FEATURES", "LightCoronas", false)) {
                if (c.corona.lightingType == eLightingMode::NonDirectional) {
                    RenderUtil::RegisterCorona(pVeh, (reinterpret_cast<unsigned int>(pVeh) * 255) + 255 + (int)type, c.position, c.corona.color, c.corona.size);
                } else {
                    RenderUtil::RegisterCoronaDirectional(&dummy->Get(), c.rotation.angle, 180.0f, 1.0f, c.corona.lightingType == eLightingMode::Inversed, false);
                }
            }

            if (c.shadow.render && !texture.empty()) {
                RenderUtil::RegisterShadowDirectional(&dummy->Get(), texture, c.shadow.size);
            }
        }
    }
    ModelInfoMgr::EnableMaterial(pVeh, type);
}

bool LightManager::IsDummyAvailable(VehLightData& data, eMaterialType type) {
    return data.dummies.count(type) > 0 && !data.dummies[type].empty();
}

#include "ModelExtrasAPI.h"

extern "C"
{
    bool ME_GetVehicleLightState(CVehicle *pVeh, ME_MaterialID lightId)
    {
        return LightManager::m_VehData.Get(pVeh).bLightStates[static_cast<eMaterialType>(lightId)];
    }

    void ME_SetVehicleLightState(CVehicle *pVeh, ME_MaterialID lightId, bool state)
    {
        LightManager::m_VehData.Get(pVeh).bLightStates[static_cast<eMaterialType>(lightId)] = state;
    }

    int __declspec(dllexport) ignore4(int i)
    {
        return 1;
    }
}
