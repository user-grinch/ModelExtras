#pragma once
#include "../data.h"
#include "core/base.h"

class BaseLightComponent {
public:
    virtual ~BaseLightComponent() = default;

    virtual eMaterialType GetMatType(CRGBA matCol) { return eMaterialType::UnknownMaterial; }
    virtual bool TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string& name, VehLightData& data) { return false; }
    virtual void Process(CVehicle* pVeh, VehLightData& data) {}
    virtual void Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {}
};
