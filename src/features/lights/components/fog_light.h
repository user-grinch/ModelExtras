#pragma once
#include "../manager.h"

class FogLightComponent {
public:
    static eMaterialType GetMatType(CRGBA matCol);
    static bool TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data);

    static void Process(CVehicle* pVeh, VehLightData& data);
    static void Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data);
};
