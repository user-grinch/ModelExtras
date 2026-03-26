#pragma once
#include "plugin.h"
#include <unordered_map>

using namespace plugin;

extern struct ME_ExhaustInfo;

struct ExhaustData {
public:
    RwFrame *pFrame = nullptr;
    std::string sName = "";
    CRGBA Color{230, 230, 230, 64};
    float fSpeedMul = 0.5f;
    float fLifeTime = 0.2f;
    float fSizeMul = 0.5f;
    bool bNitroEffect = true;
    FxSystem_c *pFxSysem = nullptr;
};

struct VehData {
    bool isUsed = false;
    size_t reloadCount = 0;
    std::unordered_map<std::string, ExhaustData> m_pDummies;
    VehData(CVehicle *pVeh) { isUsed = false; }

    ~VehData() {
    }
};

// Function types
using ExhaustFn_t = void (__fastcall *)(CVehicle *);
using NitroFn_t = char (__fastcall *)(CAutomobile *pVeh, float power);

class ExhaustFx {
private:
    static inline bool bEnabled = false;
    static inline size_t nReloadCount = 0;

    static void RenderSmokeFx(CVehicle *pVeh, const ExhaustData &info);

    static void RenderNitroFx(CVehicle *pVeh, float power);

    static ExhaustData LoadData(CVehicle *pVeh, RwFrame *pFrame);

    static void __fastcall hkAddExhaustParticles1(CVehicle *pVeh);

    static void __fastcall hkAddExhaustParticles2(CVehicle *pVeh);

    // Fixed Nitro hooks with edx parameter
    static char __fastcall hkDoNitroEffect1(CAutomobile *pVeh, float power);

    static char __fastcall hkDoNitroEffect2(CAutomobile *pVeh, float power);

    static char __fastcall hkDoNitroEffect3(CAutomobile *pVeh, float power);

    static void FindNodes(CVehicle *pVeh, RwFrame *frame);

public:
    static inline VehicleExtendedData<VehData> xData;

    static void Initialize();

    static void Reload();
};
