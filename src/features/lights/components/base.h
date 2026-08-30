#pragma once
#include "../data.h"
#include "core/base.h"
#include <unordered_map>

class BaseLightComponent {
public:
    virtual ~BaseLightComponent() = default;

    virtual void RegisterMaterials(std::unordered_map<uint32_t, eMaterialType>& matMap) {}
    virtual eMaterialType GetMatType(CRGBA matCol) { return eMaterialType::UnknownMaterial; }
    virtual bool TryRegisterDummy(CVehicle* pVeh, RwFrame* pFrame, const std::string_view name, VehLightData& data) { return false; }
    virtual void Process(CVehicle* pVeh, VehLightData& data) {}
    virtual void Render(CVehicle* pControlVeh, CVehicle* pTowedVeh, VehLightData& data) {}
    virtual void ProcessPointLights(CVehicle* pVeh, VehLightData& data) {}
};
