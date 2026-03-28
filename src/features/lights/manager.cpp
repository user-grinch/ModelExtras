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
#include "components/day_light.h"
#include "components/night_light.h"
#include "components/allday_light.h"
#include "components/side_light.h"
#include "components/spot_light.h"

void LightManager::Init() {
    m_Components.push_back(std::make_unique<HeadlightComponent>());
    m_Components.push_back(std::make_unique<IndicatorComponent>());
    m_Components.push_back(std::make_unique<RearLightsComponent>());
    m_Components.push_back(std::make_unique<FogLightComponent>());
    m_Components.push_back(std::make_unique<StrobeLightComponent>());
    m_Components.push_back(std::make_unique<DayLightComponent>());
    m_Components.push_back(std::make_unique<NightLightComponent>());
    m_Components.push_back(std::make_unique<AllDayLightComponent>());
    m_Components.push_back(std::make_unique<SideLightComponent>());
    m_Components.push_back(std::make_unique<SpotLightComponent>());
}

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

    for (const auto& comp : m_Components) {
        eMaterialType type = comp->GetMatType(matCol);
        if (type != eMaterialType::UnknownMaterial) return type;
    }

    return eMaterialType::UnknownMaterial;
}

void LightManager::RegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name) {
    VehLightData& data = m_VehData.Get(pVeh);

    for (const auto& comp : m_Components) {
        if (comp->TryRegisterDummy(pVeh, pFrame, name, data)) return;
    }
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
    for (const auto& comp : m_Components) {
        comp->Process(pVeh, data);
    }
}

void LightManager::Render(CVehicle* pControlVeh, CVehicle* pTowedVeh) {
    if (Util::IsEngineOff(pControlVeh)) return;

    VehLightData& data = m_VehData.Get(pControlVeh);
    for (const auto& comp : m_Components) {
        comp->Render(pControlVeh, pTowedVeh, data);
    }
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
    bool ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId)
    {
        return LightManager::m_VehData.Get(pVeh).bLightStates[static_cast<eMaterialType>(lightId)];
    }

    void ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state)
    {
        LightManager::m_VehData.Get(pVeh).bLightStates[static_cast<eMaterialType>(lightId)] = state;
    }

    int __declspec(dllexport) ignore4(int i)
    {
        return 1;
    }
}
