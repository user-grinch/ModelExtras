#include "pch.h"
#include "exhausts.h"
#include <CWorld.h>
#include <CCamera.h>
#include <CGeneral.h>
#include <CWaterLevel.h>
#include <Fx_c.h>

#include "utils/modelinfomgr.h"
#include "utils/datamgr.h"
#include "ModelExtrasAPI.h"
#include "backfire.h"
#include "enums/vehdummy.h"
#include "utils/meevents.h"
#include <CPointLights.h>
#include <rwcore.h>
#include <rwplcore.h>

#define NODE_NAME "x_exhaust"

// Global trampolines
ExhaustFn_t ogFunc1 = nullptr, ogFunc2 = nullptr;
NitroFn_t ogNitro1 = nullptr, ogNitro2 = nullptr, ogNitro3 = nullptr;

void __fastcall ExhaustFx::hkAddExhaustParticles1(CVehicle * pVeh)
{
    if (!pVeh) return;
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::ExhaustFx)) {
        if (ogFunc1) ogFunc1(pVeh);
        return;
    }
    auto &data = m_VehData.Get(pVeh);
    if (!data.isUsed && ogFunc1) {
        ogFunc1(pVeh);
    }
}

void __fastcall ExhaustFx::hkAddExhaustParticles2(CVehicle *pVeh)
{
    if (!pVeh) return;
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::ExhaustFx)) {
        if (ogFunc2) ogFunc2(pVeh);
        return;
    }
    auto &data = m_VehData.Get(pVeh);
    if (!data.isUsed && ogFunc2) {
        ogFunc2(pVeh);
    }
}

char __fastcall ExhaustFx::hkDoNitroEffect1(CAutomobile* pVeh, float power)
{
    auto& data = m_VehData.Get(pVeh);
    if (pVeh->m_fGasPedal > 0.05f) {
        data.lastNitroFrame = CTimer::m_FrameCounter;
    }
    if (data.isUsed) {
        RenderNitroFx(pVeh, power);
        return 1;
    }
    return ogNitro1(pVeh, power);
}

char __fastcall ExhaustFx::hkDoNitroEffect2(CAutomobile* pVeh, float power)
{
    auto& data = m_VehData.Get(pVeh);
    if (pVeh->m_fGasPedal > 0.05f) {
        data.lastNitroFrame = CTimer::m_FrameCounter;
    }
    if (data.isUsed) {
        RenderNitroFx(pVeh, power);
        return 1;
    }
    return ogNitro2(pVeh, power);
}

char __fastcall ExhaustFx::hkDoNitroEffect3(CAutomobile* pVeh, float power)
{
    auto& data = m_VehData.Get(pVeh);
    if (pVeh->m_fGasPedal > 0.05f) {
        data.lastNitroFrame = CTimer::m_FrameCounter;
    }
    if (data.isUsed) {
        RenderNitroFx(pVeh, power);
        return 1;
    }
    return ogNitro3(pVeh, power);
}

void ExhaustFx::FindNodes(CVehicle *pVeh, RwFrame *pFrame)
{
    if (pFrame)
    {
        std::string_view name = GetSafeFrameNodeName(pFrame);
        if (name.starts_with(NODE_NAME))
        {
            auto &data = m_VehData.Get(pVeh);
            data.isUsed = true;
            data.m_pDummies.emplace_back(std::string(name), LoadData(pVeh, pFrame));
        }

        if (RwFrame *newFrame = pFrame->child)
        {
            FindNodes(pVeh, newFrame);
        }
        if (RwFrame *newFrame = pFrame->next)
        {
            FindNodes(pVeh, newFrame);
        }
    }
    return;
}

void ExhaustFx::Init()
{
    bEnabled = true;

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {

        if (!pVeh || !pVeh->GetIsOnScreen()) {
            return;
        }

        ExhaustVehData &data = m_VehData.Get(pVeh);

        // Must be here to work with VehFuncs recursive extras
        if (!data.isUsed) {
            RwFrame *pFrame = (RwFrame*)pVeh->m_pRwClump->object.parent;
            FindNodes(pVeh, pFrame);
        }

        for (auto& e : data.m_pDummies) {
            RenderSmokeFx(pVeh, e.second);
            if (e.second.pFxSysem && data.lastNitroFrame != CTimer::m_FrameCounter) {
                if (e.second.pFxSysem->m_nPlayStatus == eFxSystemPlayStatus::FX_PLAYING) {
                    e.second.pFxSysem->Stop();
                }
            }
        } });
    MEEvents::vehPreRenderEvent.before += [](CVehicle *pVeh)
    {
        ExhaustFx::ProcessPointLights(pVeh);
    };



    ogFunc1 = injector::GetBranchDestination(0x6AB344, true).get();
    injector::MakeCALL(0x6AB344, hkAddExhaustParticles1, true);

    ogFunc2 = injector::GetBranchDestination(0x6BD3FF, true).get();
    injector::MakeCALL(0x6BD3FF, hkAddExhaustParticles2, true);

    ogNitro1 = injector::GetBranchDestination(0x6A405A, true).get();
    injector::MakeCALL(0x6A405A, hkDoNitroEffect1, true);

    ogNitro2 = injector::GetBranchDestination(0x6A406B, true).get();
    injector::MakeCALL(0x6A406B, hkDoNitroEffect2, true);

    ogNitro3 = injector::GetBranchDestination(0x6A40E1, true).get();
    injector::MakeCALL(0x6A40E1, hkDoNitroEffect3, true);
}

void ExhaustFx::ProcessPointLights(CVehicle *pVeh)
{
    extern bool gbLightPointLights;
    if (!gbLightPointLights || !pVeh || !pVeh->GetIsOnScreen() || !pVeh->bEngineOn || pVeh->bEngineBroken)
    {
        return;
    }

    ExhaustVehData &data = m_VehData.Get(pVeh);
    bool bNitroActive = (data.lastNitroFrame == CTimer::m_FrameCounter) ||
                        (CTimer::m_FrameCounter > 0 && data.lastNitroFrame == CTimer::m_FrameCounter - 1);

    if (!bNitroActive || pVeh->m_fGasPedal <= 0.05f)
    {
        return;
    }

    float nitroScale = std::clamp(pVeh->m_fGasPedal, 0.5f, 1.0f);
    const float radius = 0.70f * nitroScale;
    const float r = 0.0f;
    const float g = 0.45f * nitroScale;
    const float b = 1.0f;
    const float rearOffset = 0.25f;

    if (data.isUsed && !data.m_pDummies.empty())
    {
        for (const auto &e : data.m_pDummies)
        {
            if (e.second.bNitroEffect && e.second.pFrame)
            {
                CVector pos = e.second.pFrame->ltm.pos;
                if (!pos.IsZero())
                {
                    CVector backwardDir = -(CVector &)e.second.pFrame->ltm.up;
                    backwardDir.Normalize();
                    CVector plightPos = pos + backwardDir * rearOffset;
                    CPointLights::AddLight(PLTYPE_POINTLIGHT, plightPos, CVector(0.0f, 0.0f, 0.0f), radius, r, g, b, 0, false, nullptr);
                }
            }
        }
    }
    else
    {
        CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
        if (pInfo && pInfo->m_pVehicleStruct)
        {
            CVector pos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST];
            if (!pos.IsZero())
            {
                CVector worldPos = pVeh->TransformFromObjectSpace(CVector(pos.x, pos.y - rearOffset, pos.z));
                CPointLights::AddLight(PLTYPE_POINTLIGHT, worldPos, CVector(0.0f, 0.0f, 0.0f), radius, r, g, b, 0, false, nullptr);

                if (pVeh->m_pHandlingData && pVeh->m_pHandlingData->m_bDoubleExhaust)
                {
                    CVector doubleWorldPos = pVeh->TransformFromObjectSpace(CVector(-pos.x, pos.y - rearOffset, pos.z));
                    CPointLights::AddLight(PLTYPE_POINTLIGHT, doubleWorldPos, CVector(0.0f, 0.0f, 0.0f), radius, r, g, b, 0, false, nullptr);
                }
            }

            CVector secPos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST_SECONDARY];
            if (!secPos.IsZero())
            {
                CVector secWorldPos = pVeh->TransformFromObjectSpace(CVector(secPos.x, secPos.y - rearOffset, secPos.z));
                CPointLights::AddLight(PLTYPE_POINTLIGHT, secWorldPos, CVector(0.0f, 0.0f, 0.0f), radius, r, g, b, 0, false, nullptr);

                if (pVeh->m_pHandlingData && pVeh->m_pHandlingData->m_bDoubleExhaust)
                {
                    CVector doubleSecWorldPos = pVeh->TransformFromObjectSpace(CVector(-secPos.x, secPos.y - rearOffset, secPos.z));
                    CPointLights::AddLight(PLTYPE_POINTLIGHT, doubleSecWorldPos, CVector(0.0f, 0.0f, 0.0f), radius, r, g, b, 0, false, nullptr);
                }
            }
        }
    }
}
ExhaustData ExhaustFx::LoadData(CVehicle *pVeh, RwFrame *pFrame)
{
    ExhaustData f;
    f.sName = GetSafeFrameNodeName(pFrame);
    f.pFrame = pFrame;
    f.bNitroEffect = true;

    auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    if (jsonData.contains("exhausts") && jsonData["exhausts"].contains(f.sName))
    {
        auto &data = jsonData["exhausts"][f.sName];
        f.fLifeTime = data.value("lifetime", f.fLifeTime);
        f.fSpeedMul *= data.value("speed", 1.0f);
        f.fSizeMul = data.value("size", f.fSizeMul);
        f.bNitroEffect = data.value("nitro_effect", f.bNitroEffect);

        if (data.contains("color"))
        {
            f.Color.r = data["color"].value("red", f.Color.r);
            f.Color.g = data["color"].value("green", f.Color.g);
            f.Color.b = data["color"].value("blue", f.Color.b);
            f.Color.a = data["color"].value("alpha", f.Color.a);
        }
    }

    return f;
}

void ExhaustFx::RenderSmokeFx(CVehicle *pVeh, const ExhaustData &info)
{
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::ExhaustFx))
    {
        return;
    }
    if (!pVeh || !pVeh->GetIsOnScreen() || !pVeh->bEngineOn || pVeh->bEngineBroken)
    {
        return;
    }

    float dist = CVector::Distance(pVeh->GetPosition(), TheCamera.GetPosition());
    dist *= dist;

    if (dist > 256.0f || (dist > 64.0f && !((CTimer::m_FrameCounter + pVeh->m_nModelIndex) & 1)))
    {
        return;
    }

    CVector exhaustPos = info.pFrame->ltm.pos;
    if (exhaustPos.IsZero())
    {
        return;
    }

    auto &data = m_VehData.Get(pVeh);
    if (data.reloadCount < nReloadCount)
    {
        for (auto &e : data.m_pDummies)
        {
            e.second = LoadData(pVeh, e.second.pFrame);
        }
        data.reloadCount++;
    }

    // properties
    float moveSpeed = pVeh->m_vecMoveSpeed.Magnitude() * info.fSpeedMul;
    float life = std::max(info.fLifeTime - moveSpeed, 0.0f);
    float alpha = std::max(info.Color.a / 255.0f - moveSpeed, 0.0f);

    CVector particleDir = info.pFrame->ltm.up; // forward is up in psdk
    particleDir *= -1;

    CVector parVelocity;
    if (CVector::Dot(particleDir, pVeh->m_vecMoveSpeed) >= 0.05f)
    {
        parVelocity = pVeh->m_vecMoveSpeed * 30.0f;
    }
    else
    {
        static float randomFactor = CGeneral::GetRandomNumberInRange(-1.8f, -0.9f);
        parVelocity = randomFactor * particleDir;
    }

    bool isExhaustSubmerged = false;
    float waterLevel = 0.0f;
    if (pVeh->bTouchingWater &&
        CWaterLevel::GetWaterLevel(exhaustPos.x, exhaustPos.y, exhaustPos.z, &waterLevel, true, nullptr) &&
        waterLevel >= exhaustPos.z)
    {
        isExhaustSubmerged = true;
    }

    float randomFactor = CGeneral::GetRandomNumberInRange(1.0f, 3.0f);
    if (randomFactor * (pVeh->m_fGasPedal + 1.1f) <= 2.5f)
    {
        return;
    }

    FxPrtMult_c fxPrt(info.Color.r / 255.0f, info.Color.g / 255.0f, info.Color.b / 255.0f, alpha, 0.2f * info.fSizeMul, 1.0f, life);

    for (int i = 0; i < 2; i++)
    {
        FxSystem_c *fxSystem = isExhaustSubmerged ? g_fx.m_pPrtBubble : g_fx.m_pPrtSmokeII3expand;

        if (isExhaustSubmerged)
        {
            fxPrt.m_color.alpha = alpha * 0.5f;
            fxPrt.m_fSize = 0.6f * info.fSizeMul;
        }

        fxSystem->AddParticle(
            (RwV3d *)&exhaustPos,
            (RwV3d *)&parVelocity,
            0.0f,
            &fxPrt,
            -1.0f,
            pVeh->m_fContactSurfaceBrightness,
            0.6f,
            0);

        // secondary emission
        if (pVeh->m_fGasPedal > 0.5f && pVeh->m_nCurrentGear < 3 && (CGeneral::GetRandomNumber() % 2))
        {
            FxSystem_c *secondaryFxSystem = isExhaustSubmerged ? g_fx.m_pPrtBubble : g_fx.m_pPrtSmokeII3expand;

            if (isExhaustSubmerged)
            {
                fxPrt.m_color.alpha = alpha * 0.5f;
                fxPrt.m_fSize = 0.6f * info.fSizeMul;
            }

            secondaryFxSystem->AddParticle(
                (RwV3d *)&exhaustPos,
                (RwV3d *)&parVelocity,
                0.0f,
                &fxPrt,
                -1.0f,
                pVeh->m_fContactSurfaceBrightness,
                0.6f,
                0);
        }
    }
}

void ExhaustFx::RenderNitroFx(CVehicle *pVeh, float power)
{
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::ExhaustFx))
    {
        return;
    }
    const auto &mi = CModelInfo::GetModelInfo(pVeh->m_nModelIndex);

    auto &data = m_VehData.Get(pVeh);

    if (!data.isUsed || !pVeh->bEngineOn || pVeh->bEngineBroken)
    {
        return;
    }

    data.lastNitroFrame = CTimer::m_FrameCounter;

    for (auto &e : data.m_pDummies)
    {
        if (!e.second.bNitroEffect)
        {
            continue;
        }

        RwMatrix *dummyMatrix = &e.second.pFrame->ltm;

        bool isExhaustSubmerged = false;
        if (pVeh->bTouchingWater)
        {
            float level = 0.0f;
            CVector pos = dummyMatrix->pos;
            if (CWaterLevel::GetWaterLevel(pos.x, pos.y, pos.z, &level, true, nullptr))
            {
                if (level >= pos.z)
                {
                    isExhaustSubmerged = true;
                }
            }
        }

        if (e.second.pFxSysem)
        {
            e.second.pFxSysem->SetConstTime(1, std::fabs(power));
            if (e.second.pFxSysem->m_nPlayStatus == eFxSystemPlayStatus::FX_PLAYING && isExhaustSubmerged)
            {
                e.second.pFxSysem->Stop();
            }
            else if (e.second.pFxSysem->m_nPlayStatus == eFxSystemPlayStatus::FX_STOPPED && !isExhaustSubmerged)
            {
                e.second.pFxSysem->Play();
            }
        }
        else if (!isExhaustSubmerged && dummyMatrix)
        {
            static RwMatrixTag gFlipForward = {
                {1.0f, 0.0f, 0.0f},  // right (X)
                0,                   // flags
                {0.0f, -1.0f, 0.0f}, // up (forward flipped)
                0,
                {0.0f, 0.0f, 1.0f}, // at
                0,
                {0.0f, 0.0f, 0.0f}, // pos
                0};

            e.second.pFxSysem = g_fxMan.CreateFxSystem((char *)"nitro", &gFlipForward, dummyMatrix, true);
            if (e.second.pFxSysem)
            {
                e.second.pFxSysem->SetLocalParticles(true);
                e.second.pFxSysem->Play();
            }
        }
    }
}

void ExhaustFx::Reload(CVehicle* pVeh)
{
    if (pVeh) {
        auto &data = m_VehData.Get(pVeh);
        for (auto &e : data.m_pDummies) {
            if (e.second.pFxSysem) {
                e.second.pFxSysem->Kill();
                e.second.pFxSysem = nullptr;
            }
        }
    }
    nReloadCount++;
}

#ifdef __cplusplus
extern "C"
{
#endif
    unsigned int ME_GetExhaustCount(CVehicle *pVeh)
    {
        if (!pVeh)
            return 0;

        ExhaustVehData &data = ExhaustFx::m_VehData.Get(pVeh);
        if (!data.isUsed)
            return 0;

        return static_cast<unsigned int>(data.m_pDummies.size());
    }

    ME_ExhaustInfo ME_GetExhaustData(CVehicle *pVeh, int index)
    {
        ME_ExhaustInfo info{};
        if (!pVeh)
            return info;

        ExhaustVehData &data = ExhaustFx::m_VehData.Get(pVeh);
        if (!data.isUsed || index < 0 || index >= static_cast<int>(data.m_pDummies.size()))
            return info;

        const ExhaustData &e = data.m_pDummies[index].second;
        info.pFrame = e.pFrame;
        info.Color = { e.Color.r, e.Color.g, e.Color.b, e.Color.a };
        info.fSpeedMul = e.fSpeedMul;
        info.fLifeTime = e.fLifeTime;
        info.fSizeMul = e.fSizeMul;
        info.bNitroEffect = e.bNitroEffect;

        return info;
    }

    void ME_SetExhaustData(CVehicle *pVeh, int index, ME_ExhaustInfo &data)
    {
        if (!pVeh)
            return;

        ExhaustVehData &vData = ExhaustFx::m_VehData.Get(pVeh);
        if (!vData.isUsed || index < 0 || index >= static_cast<int>(vData.m_pDummies.size()))
            return;

        ExhaustData &e = vData.m_pDummies[index].second;
        e.pFrame = data.pFrame;
        e.Color = CRGBA(data.Color.r, data.Color.g, data.Color.b, data.Color.a);
        e.fSpeedMul = data.fSpeedMul;
        e.fLifeTime = data.fLifeTime;
        e.fSizeMul = data.fSizeMul;
        e.bNitroEffect = data.bNitroEffect;
    }

    // Dummy function to show on crash logs
    int __declspec(dllexport) ignore3(int i)
    {
        return 1;
    }

#ifdef __cplusplus
}
#endif