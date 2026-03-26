#pragma once
#include "plugin.h"
#include "core/base.h"
#include <unordered_map>
#include <Fx_c.h>

using namespace plugin;

extern struct ME_ExhaustInfo;





// Function types
using ExhaustFn_t = void (__fastcall *)(CVehicle *);
using NitroFn_t = char (__fastcall *)(CAutomobile *pVeh, float power);

struct ExhaustData
{
    std::string sName;
    RwFrame *pFrame = nullptr;
    CRGBA Color = {150, 150, 150, 200}; // Dark grey default
    float fLifeTime = 1.0f;             // Longer lifetime for larger pipes
    float fSpeedMul = 1.0f;             // Speed multiplier
    float fSizeMul = 1.0f;
    bool bNitroEffect = false;
    FxSystem_c *pFxSysem = nullptr;
};

struct ExhaustVehData {
    bool isUsed = false;
    size_t reloadCount = 0;
    std::unordered_map<std::string, ExhaustData> m_pDummies;
    ExhaustVehData(CVehicle *pVeh) { isUsed = false; }

    ~ExhaustVehData() {
    }
};

class ExhaustFx : public CVehFeature<ExhaustVehData>
{
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

protected:
    void Init() override;

public:

    public:
    ExhaustFx() : CVehFeature<ExhaustVehData>("ExhaustFx", "FEATURES", eFeatureMatrix::ExhaustFx) {}

    void Reload(CVehicle* pVeh) override;
};
