#pragma once
#include <array>
#include <extender/PedExtender.h>
#include <CRGBA.h>
#include "core/base.h"

struct RpAtomic;
struct RpClump;
struct RpMaterial;

using namespace plugin;

class PedData {
public:
    std::array<CRGBA, 4> m_Colors = {CRGBA(255, 255, 255), CRGBA(255, 255, 255), CRGBA(255, 255, 255), CRGBA(255, 255, 255)};
    std::vector<std::pair<RwRGBA*, RwRGBA>> m_OriginalColors;
    bool m_bUsingPedCols = false;

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
