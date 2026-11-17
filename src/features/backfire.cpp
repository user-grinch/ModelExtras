#include "pch.h"
#include "defines.h"
#include "backfire.h"
#include "utils/audiomgr.h"
#include "enums/vehdummy.h"
#include "ModelExtrasAPI.h"
#include <extensions/ScriptCommands.h>
#include <CTheScripts.h>
#include <FxSystem_c.h>
#include <CGeneral.h>
#include <CAutomobile.h>
#include <CBike.h>
#include <vector>
#include <algorithm>

namespace
{
std::vector<int> s_ValidModels;
bool s_bOnlySelected = true;
bool s_bDisabledByConflict = false;

inline float GetThrottle(CVehicle *pVeh)
{
    if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
    {
        return static_cast<CAutomobile *>(pVeh)->m_fGasPedalAudioRevs;
    }
    if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
    {
        return static_cast<CBike *>(pVeh)->m_fGasPedalAudioRevs;
    }
    return std::abs(pVeh->m_fGasPedal);
}

inline CVector WorldToObjectSpace(CVehicle *pVeh, const CVector &worldPos)
{
    CVector diff = worldPos - pVeh->GetPosition();
    const CMatrix &mat = pVeh->GetMatrix();
    return CVector(
        diff.x * mat.GetRight().x + diff.y * mat.GetRight().y + diff.z * mat.GetRight().z,
        diff.x * mat.GetForward().x + diff.y * mat.GetForward().y + diff.z * mat.GetForward().z,
        diff.x * mat.GetUp().x + diff.y * mat.GetUp().y + diff.z * mat.GetUp().z
    );
}

inline FxSystem_c *GetScriptFxSystem(int fxHandle)
{
    if (fxHandle <= 0) return nullptr;
    int index = fxHandle & 0xFFFF;
    int uniqueId = (fxHandle >> 16) & 0xFFFF;
    if (index >= 0 && index < 32 && CTheScripts::ScriptEffectSystemArray)
    {
        auto &slot = CTheScripts::ScriptEffectSystemArray[index];
        if (slot.bUsed && slot.wUniqueID == uniqueId)
        {
            return static_cast<FxSystem_c *>(slot.m_pFxSystem);
        }
    }
    return nullptr;
}
}

void BackFireEffect::BackFireSingle(CVehicle *pVeh, bool bPlaySound, bool bHighIntensity)
{
    if (s_bDisabledByConflict || !pVeh || !pVeh->m_pRwClump || !CBaseFeature::IsEnabled(eFeatureMatrix::BackfireEffect)) return;

    std::vector<CVector> exhaustOffsets;
    CVector soundPos(0.0f, -2.0f, 0.0f);

    size_t customCount = ME_GetExhaustCount(pVeh);
    if (customCount > 0)
    {
        CVector soundAccum(0.0f, 0.0f, 0.0f);
        for (size_t i = 0; i < customCount; i++)
        {
            const ME_ExhaustInfo &info = ME_GetExhaustData(pVeh, static_cast<int>(i));
            if (info.pFrame)
            {
                CVector localPos = WorldToObjectSpace(pVeh, info.pFrame->ltm.pos);
                exhaustOffsets.push_back(localPos);
                soundAccum += localPos;
            }
        }
        if (!exhaustOffsets.empty())
        {
            soundPos = soundAccum / static_cast<float>(exhaustOffsets.size());
        }
    }
    else
    {
        CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(GetSafeModelInfo(pVeh->m_nModelIndex));
        if (pInfo && pInfo->m_pVehicleStruct)
        {
            CVector dummyPos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST];
            bool isDouble = false;
            int extra0 = pVeh->m_anExtras[0];

            // Motorcycle dual-exhaust extra variations
            if (pVeh->m_nModelIndex == MODEL_FCR900 && extra0 == 1)
            {
                isDouble = true;
            }
            else if (pVeh->m_nModelIndex == MODEL_BF400 && extra0 == 2)
            {
                isDouble = true;
            }
            else if (pVeh->m_pHandlingData)
            {
                isDouble = pVeh->m_pHandlingData->m_bDoubleExhaust;
            }

            // NRG-500 asymmetric dual exhaust using dummy 6 (EXHAUST) + dummy 11 (EXHAUST_SECONDARY)
            if (pVeh->m_nModelIndex == MODEL_NRG500 && (extra0 == 0 || extra0 == 1))
            {
                CVector secDummyPos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST_SECONDARY];
                exhaustOffsets.push_back(dummyPos);
                exhaustOffsets.push_back(secDummyPos);
                soundPos = (dummyPos + secDummyPos) * 0.5f;
            }
            else
            {
                exhaustOffsets.push_back(dummyPos);
                if (isDouble)
                {
                    exhaustOffsets.push_back(CVector(-dummyPos.x, dummyPos.y, dummyPos.z));
                    soundPos = CVector(0.0f, dummyPos.y, dummyPos.z);
                }
                else
                {
                    soundPos = dummyPos;
                }
            }
        }
    }

    if (exhaustOffsets.empty()) return;

    for (const auto &offset : exhaustOffsets)
    {
        int fxHandle = 0;
        plugin::scripting::CallCommandById(COMMAND_CREATE_FX_SYSTEM_ON_CAR_WITH_DIRECTION,
            "gunflash", pVeh, offset.x, offset.y, offset.z, 0.0f, -25.0f, 0.0f, 1, &fxHandle);
        if (fxHandle)
        {
            if (FxSystem_c *pFx = GetScriptFxSystem(fxHandle))
            {
                pFx->SetRateMult(bHighIntensity
                    ? CGeneral::GetRandomNumberInRange(0.70f, 0.90f)
                    : CGeneral::GetRandomNumberInRange(0.20f, 0.35f));
            }
            plugin::scripting::CallCommandById(COMMAND_PLAY_AND_KILL_FX_SYSTEM, fxHandle);
        }
    }

    if (bPlaySound)
    {
        static const std::string s_AudioPath = MOD_DATA_PATH("audio/backfire.wav");
        float volume = bHighIntensity ? 1.6f : 1.1f;
        float maxDist = bHighIntensity ? 80.0f : 60.0f;
        AudioMgr::Play3DSound(s_AudioPath, pVeh->TransformFromObjectSpace(soundPos), volume, maxDist);
    }
}

void BackFireEffect::BackFireMulti(CVehicle *pVeh, bool bPlaySound, bool bHighIntensity)
{
    if (s_bDisabledByConflict || !pVeh || !pVeh->m_pRwClump || !CBaseFeature::IsEnabled(eFeatureMatrix::BackfireEffect)) return;

    BackFireSingle(pVeh, bPlaySound, bHighIntensity);

    // Staggered secondary multi-bursts: 0 to 3 extra bursts with 20..300ms delays
    BackfireVehData &data = m_VehData.Get(pVeh);
    data.m_nleftFires = CGeneral::GetRandomNumberInRange(0, 4);
    data.prevTimer = CTimer::m_snTimeInMilliseconds + static_cast<uint32_t>(CGeneral::GetRandomNumberInRange(20, 300));
}

bool BackFireEffect::IsDisabledByConflict()
{
    return s_bDisabledByConflict;
}

void BackFireEffect::ReloadConfig()
{
    CVehFeature::ReloadConfig();
    s_bDisabledByConflict = GetModuleHandleA("VehFuncs.asi") && GetModuleHandleA("Soundize (Junior_Djjr).asi");
    if (s_bDisabledByConflict)
    {
        LOG(INFO) << "[Backfire] VehFuncs and Soundize detected; disabling ModelExtras backfire to prevent conflicts.";
    }
    s_bOnlySelected = gConfig.ReadBoolean("FEATURES", "BackfireEffect_OnlySelected", true);
    std::string line = gConfig.ReadString("TABLE", "BackFireEffect_VehicleModels", "");
    s_ValidModels.clear();
    Util::GetModelsFromIni(line, s_ValidModels);

    if (CPools::ms_pVehiclePool)
    {
        for (auto *pVeh : CPools::ms_pVehiclePool)
        {
            if (pVeh)
            {
                auto &data = m_VehData.Get(pVeh);
                data.wasFullThrottled = false;
                data.prevGchanging = 1;
                data.m_nleftFires = 0;
                data.prevTimer = 0;
            }
        }
    }
}

void BackFireEffect::Init()
{
    ReloadConfig();

    Events::initGameEvent += [this]()
    {
        ReloadConfig();
    };

    Events::vehicleRenderEvent.before += [](CVehicle *vehicle)
    {
        BackFireEffect::Process(vehicle);
    };
}

// Dynamic vehicle backfire simulation processing
void BackFireEffect::Process(CVehicle *pVeh)
{
    if (s_bDisabledByConflict || !pVeh || !pVeh->m_pRwClump || !CBaseFeature::IsEnabled(eFeatureMatrix::BackfireEffect)) return;

    if (!pVeh->GetIsOnScreen() || pVeh->bEngineBroken || !pVeh->bEngineOn || pVeh->bIsRCVehicle || !pVeh->m_pDriver)
    {
        return;
    }

    if (pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE && pVeh->m_nVehicleSubClass != VEHICLE_BIKE)
    {
        return;
    }

    if (s_bOnlySelected && std::find(s_ValidModels.begin(), s_ValidModels.end(), pVeh->m_nModelIndex) == s_ValidModels.end())
    {
        return;
    }

    // Skip backfire during active nitro boost
    if (*reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(pVeh) + 0x37C) != 0)
    {
        return;
    }

    BackfireVehData &data = m_VehData.Get(pVeh);
    uint32_t curTime = CTimer::m_snTimeInMilliseconds;

    // Process queued multi-burst pops
    if (data.m_nleftFires > 0 && curTime >= data.prevTimer)
    {
        BackFireSingle(pVeh, true, false);
        data.m_nleftFires--;
        data.prevTimer = curTime + static_cast<uint32_t>(CGeneral::GetRandomNumberInRange(20, 300));
    }

    float throttle = GetThrottle(pVeh);
    float speed = Util::GetVehicleSpeed(pVeh);

    // Trigger 1: Handbrake launch limiter
    if (pVeh->bIsHandbrakeOn && throttle >= 0.99f)
    {
        if (data.m_nleftFires == 0 && curTime >= data.prevTimer)
        {
            BackFireMulti(pVeh, true, true);
        }
    }

    // Trigger 2: Throttle lift-off / overrun
    if (data.wasFullThrottled)
    {
        if (throttle < 0.78f)
        {
            if (speed > 5.0f)
            {
                BackFireMulti(pVeh, true, false);
            }
            data.wasFullThrottled = false;
        }
    }
    else if (speed > 5.0f && throttle >= 0.99f)
    {
        data.wasFullThrottled = true;
    }

    // Trigger 3: Gear shift up
    uint8_t gchanging = *reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(pVeh) + 0x284);
    uint16_t rpm = *reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(pVeh) + 0x280);

    if (data.prevGchanging != 0 && gchanging == 0 && rpm != 0xFFFF && rpm > 100 && speed > 5.0f)
    {
        BackFireSingle(pVeh, true, true);
    }
    data.prevGchanging = gchanging;
}
