#pragma once
#include "data.h"
#include "core/base.h"
#include <shared/extender/VehicleExtender.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <initializer_list>
#include "components/base.h"

class LightManager {
public:
    static inline VehicleExtendedData<VehLightData> m_VehData;
    static inline std::vector<std::unique_ptr<BaseLightComponent>> m_Components;
    static inline std::unordered_map<uint32_t, eMaterialType> m_MaterialMap;

    static void Init();
    static eMaterialType GetMatType(RpMaterial* pMat);
    static void RegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name);
    
    static void Process(CVehicle* pVeh);
    static void ProcessPointLights(CVehicle* pVeh);
    static void Render(CVehicle* pControlVeh, CVehicle* pTowedVeh);

    static DummyConfig CreateBaseConfig(CVehicle* pVeh, RwFrame* pFrame);
    static void RenderLight(CVehicle* pVeh, VehLightData& data, eMaterialType type, bool isOn, const std::string& texture = "", float sz = 1.0f, bool highlight = false, bool isDummyOk = true, bool materialsOnly = false);
    static void RenderLights(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data, eMaterialType type, bool isOn, const std::string& texture = "", float sz = 1.0f, bool highlight = false, bool isDummyOk = true, bool materialsOnly = false);
    static bool IsDummyAvailable(VehLightData& data, eMaterialType type);
    static bool IsDummyAvailable(VehLightData& data, std::initializer_list<eMaterialType> types);
    static bool IsMaterialAvailable(CVehicle* pVeh, eMaterialType type);
    static bool IsMaterialAvailable(CVehicle* pVeh, std::initializer_list<eMaterialType> types);
    static void EnableDummy(int id, VehicleDummy *dummy, CVehicle *pVeh, float szMul);
    static void Reload(CVehicle* pVeh);
    static bool GetLightState(CVehicle* pVeh, eMaterialType lightId) { return m_VehData.Get(pVeh).bLightStates[lightId]; }
    static void SetLightState(CVehicle* pVeh, eMaterialType lightId, bool state) { m_VehData.Get(pVeh).bLightStates[lightId] = state; }
    static bool IsIndicatorOn(CVehicle* pVeh) {
        return pVeh->m_fHealth > 0.0f && (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_BIKE) && BlinkerState::Get().bIndicatorsDelay && m_VehData.Get(pVeh).nIndicatorState != eIndicatorState::Off;
    }
};
