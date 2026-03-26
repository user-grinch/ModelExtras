#pragma once
#include <plugin.h>
#include <extender/PedExtender.h>
#include <CRGBA.h>
#include "core/base.h"

struct RpAtomic;
struct RpClump;
struct RpMaterial;

using namespace plugin;

class PedData {
public:
    std::vector<RpMaterial*> materials;
    std::vector<CRGBA> m_Colors;
    bool m_bUsingPedCols = false;
    bool m_bInitialized = false;
    int randId = -1;

    PedData(CPed *pPed);
    ~PedData() {}
};

class PedColors : public CBaseFeature {
protected:
    void Init();

    static inline PedExtendedData<PedData> m_PedData;
    static inline CPed* m_pCurrentPed = nullptr;
    static void SetEditableMaterials(RpClump *pClump);

public:
    PedColors() : CBaseFeature("PedCols", "FEATURES", eFeatureMatrix::PedCols) {}
};
