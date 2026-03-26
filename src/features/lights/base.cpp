#include "pch.h"
#include "base.h"
#include "utils/modelinfomgr.h"

bool ILightBehaviorBase::IsDummyAvail(CVehicle* pVeh) {
    auto types = GetTypes();
    for (const auto e : types) {
        auto& data = commonData.Get(pVeh);
        if (!data.dummies[e].empty()) {
            return true;
        }
    }
    return false;
}

bool ILightBehaviorBase::IsValidDummy(RwFrame *frame) {
    std::string name = GetFrameNodeName(frame);
    return IsValidDummy(name);
}

bool ILightBehaviorBase::RegisterDummy(RwFrame *frame) {
    std::string name = GetFrameNodeName(frame);
    return RegisterDummy(name);
}

bool ILightBehaviorBase::IsValidMaterial(RpMaterial *pMat) { 
    return IsValidMaterial(Util::GetMaterialColor(pMat)); 
}

eMaterialType ILightBehaviorBase::GetMatType(RpMaterial *pMat) { 
    return GetMatType(Util::GetMaterialColor(pMat)); 
}

bool ILightBehaviorBase::IsDummyAvail(CVehicle* pVeh, eMaterialType state) {
    auto& data = commonData.Get(pVeh);
    return !data.dummies[state].empty();
}

bool ILightBehaviorBase::IsMatAvail(CVehicle* pVeh) {
    auto types = GetTypes();
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
        const VehicleDummyConfig &c = pDummy->GetRef();
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

void ILightBehaviorBase::RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh)
{
    int model = pControlVeh->m_nModelIndex;

    auto types = GetTypes();
    for (const auto e : types) {
        if (!LightsGlobal::Get().m_bLightStates[e])
        {
            continue;
        }

        auto &controlData = commonData.Get(pControlVeh);

        RenderLight(pControlVeh, e);
        
        if (pControlVeh != pTowedVeh)
        {
            auto &towedData = commonData.Get(pTowedVeh);
            RenderLight(pTowedVeh, e);
        }
    }
}

void ILightBehaviorBase::RenderLight(CVehicle *pVeh, eMaterialType state)
{
    bool litMats = true;
    
    if (IsDummyAvail(pVeh, state)) 
    {
        for (auto e : commonData.Get(pVeh).dummies[state])
        {
            const VehicleDummyConfig &dummy = e->GetRef();
            e->Update();
            RwFrame *parent = RwFrameGetParent(e->Get().frame);
            eMaterialType type = e->GetRef().lightType;
            bool atomicCheck = type != eMaterialType::HeadLightLeft && type != eMaterialType::HeadLightRight && !FrameUtil::IsOkAtomicVisible(parent);
            
            auto& data = commonData.Get(pVeh);
            if (atomicCheck || (dummy.dummyPos == eDummyPos::Rear && pVeh->m_pTrailer) || (!dummy.isParentDummy && !data.shouldRender(pVeh)))
            {
                litMats = false;
                break;
            }

            EnableDummy(pVeh, e);

            // Skip front shadows on bike wheelie
            if (dummy.dummyPos == eDummyPos::Front && Util::IsVehicleDoingWheelie(pVeh))
            {
                continue;
            }

            if (dummy.shadow.render)
            {
                RenderUtil::RegisterShadowDirectional(&e->Get(), dummy.shadow.texture, dummy.shadow.size);
            }
        }
    }

    if (litMats)
    {
        ModelInfoMgr::EnableMaterial(pVeh, state);
    }
}