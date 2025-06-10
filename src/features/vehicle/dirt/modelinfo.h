#pragma once
#include "plugin.h"
#include "CVehicleModelInfo.h"
#include "fxrender.h"

using namespace plugin;


class FxVehModelInfo : CVehicleModelInfo
{
public:
    static void Initialize();
    static void SetEditableMaterialsCB(RpMaterial *material, void *data);
    static void EmptyFindMats(CVehicleModelInfo *modelInfo);
    // static void SetCarCustomPlate(CVehicleModelInfo* modelInfo, RpClump* clump);
    static void ProcessGeometryForDirt(RpGeometry *geometry, int DirtLevel);
    static void FindEditableMaterialList(CVehicleModelInfo *modelInfo, int Dirtlevel);
    static void RemapDirt(CVehicleModelInfo *modelInfo, int DirtLevel);
};