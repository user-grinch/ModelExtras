#pragma once
#include "data.h"
#include "core/base.h"
#include <shared/extender/VehicleExtender.h>
#include <vector>
#include <memory>
#include "components/base.h"

class LightManager {
public:
    static inline VehicleExtendedData<VehLightData> m_VehData;
    static inline std::vector<std::unique_ptr<BaseLightComponent>> m_Components;

    static void Init();
    static eMaterialType GetMatType(RpMaterial* pMat);
    static void RegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name);
    
    static void Process(CVehicle* pVeh);
    static void Render(CVehicle* pControlVeh, CVehicle* pTowedVeh);

    static DummyConfig CreateBaseConfig(CVehicle* pVeh, RwFrame* pFrame);
    static void RenderLight(CVehicle* pVeh, VehLightData& data, eMaterialType type, bool isOn, const std::string& texture = "");
    static bool IsDummyAvailable(VehLightData& data, eMaterialType type);
};
