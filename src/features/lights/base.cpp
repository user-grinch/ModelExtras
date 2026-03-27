#include "pch.h"
#include "base.h"
#include "utils/modelinfomgr.h"

bool ILightBehaviorBase::IsDummyAvail(CVehicle* pVeh) {
    auto types = GetSupportedMatTypes();
    for (const auto e : types) {
        auto& data = commonData.Get(pVeh);
        if (!data.dummies[e].empty()) {
            return true;
        }
    }
    return false;
}

bool ILightBehaviorBase::IsDummyAvail(CVehicle* pVeh, eMaterialType state) {
    auto& data = commonData.Get(pVeh);
    return !data.dummies[state].empty();
}

bool ILightBehaviorBase::IsMatAvail(CVehicle* pVeh) {
    auto types = GetSupportedMatTypes();
    for (const auto e : types) {
        if (ModelInfoMgr::IsMaterialAvailable(pVeh, e)) {
            return true;
        }
    }
    return false;
}

void ILightBehaviorBase::EnableDummy(CVehicle *pVeh, VehicleDummy *pDummy)
{
    if (gConfig.ReadBoolean("FEATURES", "LightCoronas", false))
    {
        const DummyConfig &c = pDummy->GetRef();
        if (c.corona.lightingType == eLightingMode::NonDirectional)
        {
            RenderUtil::RegisterCorona(pVeh, c.id, c.position, c.corona.color, c.corona.size);
        }
        else
        {
            RenderUtil::RegisterCoronaDirectional(&pDummy->Get(), c.rotation.angle, 180.0f, 1.0f, c.corona.lightingType == eLightingMode::Inversed, false);
        }
    }
}

template <typename T>
void ILightBehavior<T>::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh)
{
    int model = pControlVeh->m_nModelIndex;

    auto types = GetSupportedMatTypes();
    for (const auto e : types) {
        RenderLight(pControlVeh, e);
        
        if (pControlVeh != pTowedVeh)
        {
            RenderLight(pTowedVeh, e);
        }
    }
}

template <typename T>
void ILightBehavior<T>::RenderLight(CVehicle *pVeh, eMaterialType state)
{
    if (IsDummyAvail(pVeh, state)) 
    {
        auto& typeData = GetTypeData().Get(pVeh);

        for (auto e : typeData.dummies[state])
        {
            const DummyConfig &c = e->GetRef();
            e->Update();
            
            EnableDummy(pVeh, e);

            // Skip front shadows on bike wheelie
            if (c.dummyPos == eDummyPos::Front && Util::IsVehicleDoingWheelie(pVeh))
            {
                continue;
            }

            if (c.shadow.render)
            {
                // Textures aren't customizable
                RenderUtil::RegisterShadowDirectional(&e->Get(), typeData.texName, c.shadow.size);
            }
        }
    }

    ModelInfoMgr::EnableMaterial(pVeh, state);
}