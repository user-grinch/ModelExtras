#include "pch.h"
#include "modelinfo.h"
#include <chrono>
#include "rwcore.h"
#include "rpworld.h"
#include "rwplcore.h"
#include "RenderWare.h"
#include "fxrender.h"

RwObjectNameIdAssocation *CVehicleModelInfo::ms_vehicleDescs = (RwObjectNameIdAssocation *)0x8A7740;

void FxVehModelInfo::Initialize()
{
    FxRender::Initialize();
    // Patch New Dirt Materials

    // patch::RedirectJump(0x5D5DB0, FxVehModelInfo::RemapDirt);
    patch::RedirectCall(0x6D0E7E, FxVehModelInfo::RemapDirt);
    patch::RedirectCall(0x4C9648, &FxVehModelInfo::EmptyFindMats);
}

void FxVehModelInfo::SetEditableMaterialsCB(RpMaterial *material, void *data)
{
    tRestoreEntry **pEntries = (tRestoreEntry **)data;
    int CarColorID;
    int LightID = -1;
    // int MaterialColor = *(int*)&material->color & 0xFFFFFF;
    CRGBA MaterialColor = material->color;
    int LighttextureType = 0;
    bool IsBroken = false;

    if (ms_pRemapTexture && material && RpMaterialGetTexture(material) && material->texture->name[0] == '#')
    {
        (*pEntries)->Address = &material->texture;
        (*pEntries)->Value = material->texture;
        (*pEntries)++;
        material->texture = ms_pRemapTexture;
    }
}

void FxVehModelInfo::EmptyFindMats(CVehicleModelInfo *modelInfo)
{
    // We don't do anything here atm.
}

void FxVehModelInfo::ProcessGeometryForDirt(RpGeometry *geometry, int DirtLevel)
{
    if (!geometry)
        return;

    for (int i = 0; i < geometry->matList.numMaterials; ++i)
    {
        RpMaterial *material = geometry->matList.materials[i];
        if (!material)
            continue;

        RwTexture *texture = RpMaterialGetTexture(material);
        if (!texture)
            continue;

        std::string texName = RwTextureGetName(texture);
        if (texName.empty())
            continue;

        if (texName == "vehiclegrunge256")
            RpMaterialSetTexture(material, FxRender::ms_aDirtTextures[DirtLevel]);
        else if (texName == "vehicle_genericmud_truck" || texName == "vehiclegrunge_iv")
            RpMaterialSetTexture(material, FxRender::ms_aDirtTextures_2[DirtLevel]);
        else if (texName == "vehiclegrunge512")
            RpMaterialSetTexture(material, FxRender::ms_aDirtTextures_3[DirtLevel]);
        else if (texName.starts_with("tyrewall_dirt"))
            RpMaterialSetTexture(material, FxRender::ms_aDirtTextures_4[DirtLevel]);
    }
}

void FxVehModelInfo::FindEditableMaterialList(CVehicleModelInfo *modelInfo, int DirtLevel)
{
    if (!modelInfo || !modelInfo->m_pRwObject)
        return;

    RpClump *clump = reinterpret_cast<RpClump *>(modelInfo->m_pRwObject);

    for (RwLLLink *link = rwLinkListGetFirstLLLink(&clump->atomicList);
         link != rwLinkListGetTerminator(&clump->atomicList);
         link = rwLLLinkGetNext(link))
    {
        RpAtomic *atomic = rwLLLinkGetData(link, RpAtomic, inClumpLink);
        if (atomic)
            ProcessGeometryForDirt(atomic->geometry, DirtLevel);
    }

    if (!modelInfo->m_pVehicleStruct)
        return;

    for (uint32_t i = 0; i < modelInfo->m_pVehicleStruct->m_nNumExtras; ++i)
    {
        RpAtomic *extra = modelInfo->m_pVehicleStruct->m_apExtras[i];
        if (extra)
            ProcessGeometryForDirt(extra->geometry, DirtLevel);
    }
}

void FxVehModelInfo::RemapDirt(CVehicleModelInfo *modelInfo, int DirtLevel)
{
    FindEditableMaterialList(modelInfo, DirtLevel); // takes only 0.005ms so its fineeeee
}
