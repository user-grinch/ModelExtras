#pragma once
#include "base.h"
#include "../manager.h"

class FogLightComponent : public BaseLightComponent {
public:
    eMaterialType GetMatType(CRGBA matCol) override;
    bool TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) override;

    void Process(CVehicle* pVeh, VehLightData& data) override;
    void Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) override;
};
