#include "pch.h"
#include "defines.h"
#include "backfire.h"
#include "utils/datamgr.h"
#include "utils/audiomgr.h"
#include "enums/vehdummy.h"
#include <CGeneral.h>
#include <Fx_c.h>
#include "ModelExtrasAPI.h"

using namespace plugin;

namespace
{
using tExt_IsVehicleUsingAnyBank = int (__cdecl *)(CVehicle *vehicle);
using tExt_DoesVehicleSupportBackfire = int (__cdecl *)(CVehicle *vehicle);
using tExt_GetVehicleDoingBackfire = int (__cdecl *)(CVehicle *vehicle);
using tExt_GetVehicleGear = int (__cdecl *)(CVehicle *vehicle);

struct VehFuncsCompat
{
    static bool IsPresent()
    {
        return GetModuleHandleA("VehFuncs.asi") != nullptr;
    }
};

struct SoundizeAPI
{
    static inline HMODULE hSoundize = nullptr;
    static inline bool bInitialized = false;

    static inline tExt_IsVehicleUsingAnyBank fnIsVehicleUsingAnyBank = nullptr;
    static inline tExt_DoesVehicleSupportBackfire fnDoesVehicleSupportBackfire = nullptr;
    static inline tExt_GetVehicleDoingBackfire fnGetVehicleDoingBackfire = nullptr;
    static inline tExt_GetVehicleGear fnGetVehicleGear = nullptr;

    static void Init()
    {
        if (bInitialized)
        {
            return;
        }
        bInitialized = true;

        hSoundize = GetModuleHandleA("Soundize (Junior_Djjr).asi");
        if (!hSoundize)
        {
            hSoundize = GetModuleHandleA("Soundize.asi");
        }

        if (hSoundize)
        {
            LOG(INFO) << "Soundize detected, enabling compatibility mode for ModelExtras backfire.";
            fnIsVehicleUsingAnyBank = reinterpret_cast<tExt_IsVehicleUsingAnyBank>(GetProcAddress(hSoundize, "Ext_IsVehicleUsingAnyBank"));
            fnDoesVehicleSupportBackfire = reinterpret_cast<tExt_DoesVehicleSupportBackfire>(GetProcAddress(hSoundize, "Ext_DoesVehicleSupportBackfire"));
            fnGetVehicleDoingBackfire = reinterpret_cast<tExt_GetVehicleDoingBackfire>(GetProcAddress(hSoundize, "Ext_GetVehicleDoingBackfire"));
            fnGetVehicleGear = reinterpret_cast<tExt_GetVehicleGear>(GetProcAddress(hSoundize, "Ext_GetVehicleGear"));
        }

        if (VehFuncsCompat::IsPresent())
        {
            LOG(INFO) << "VehFuncs detected, enabling exhaust dummy compatibility.";
        }
    }

    static bool IsPresent()
    {
        if (!bInitialized)
        {
            Init();
        }
        return hSoundize != nullptr;
    }
};
}

void BackfireData::CleanUpSystems()
{
    for (auto *fx : m_fxLow)
    {
        if (fx) fx->Kill();
    }
    m_fxLow.clear();

    for (auto *fx : m_fxHigh)
    {
        if (fx) fx->Kill();
    }
    m_fxHigh.clear();

    for (auto *mat : m_matrices)
    {
        if (mat) delete mat;
    }
    m_matrices.clear();

    bSystemsInitialized = false;
}

BackfireData::~BackfireData()
{
    CleanUpSystems();
}

void BackFireEffect::EnsureSystemsCreated(CVehicle *pVeh, BackfireData &data)
{
    if (data.bSystemsInitialized)
    {
        return;
    }
    data.bSystemsInitialized = true;

    FxSystemBP_c *bp = g_fxMan.FindFxSystemBP(const_cast<char *>("backfire"));
    if (!bp)
    {
        bp = g_fxMan.FindFxSystemBP(const_cast<char *>("gunflash"));
    }

    FxSystemBP_c *bpHigh = g_fxMan.FindFxSystemBP(const_cast<char *>("backfire_high"));
    if (!bpHigh)
    {
        bpHigh = bp;
    }

    if (!bp)
    {
        return;
    }

    RwMatrix *vehMat = reinterpret_cast<RwMatrix *>(pVeh->m_matrix);
    static RwV3d zAxis = { 0.0f, 0.0f, 1.0f };

    size_t count = ME_GetExhaustCount(pVeh);
    if (count <= 0)
    {
        CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
        if (!pInfo || !pInfo->m_pVehicleStruct || !pVeh->m_pHandlingData)
        {
            return;
        }

        CVector pos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST];
        bool isDouble = pVeh->m_pHandlingData->m_bDoubleExhaust;
        int total = isDouble ? 2 : 1;

        for (int i = 0; i < total; i++)
        {
            RwMatrix *mat = new RwMatrix();
            RwV3d p = { (i == 1 ? -pos.x : pos.x), pos.y, pos.z };
            RwMatrixTranslate(mat, &p, rwCOMBINEREPLACE);
            RwMatrixRotate(mat, &zAxis, 180.0f, rwCOMBINEPRECONCAT);

            data.m_fxLow.push_back(g_fxMan.CreateFxSystem(bp, mat, vehMat, true));
            data.m_fxHigh.push_back(g_fxMan.CreateFxSystem(bpHigh, mat, vehMat, true));
            data.m_matrices.push_back(mat);
        }
    }
    else
    {
        for (size_t i = 0; i < count; i++)
        {
            const ME_ExhaustInfo &info = ME_GetExhaustData(pVeh, static_cast<int>(i));
            if (info.pFrame)
            {
                RwMatrix *exhaustMatrix = RwFrameGetMatrix(info.pFrame);
                RwMatrix *mat = new RwMatrix();
                memcpy(mat, exhaustMatrix, sizeof(RwMatrix));

                data.m_fxLow.push_back(g_fxMan.CreateFxSystem(bp, mat, vehMat, true));
                data.m_fxHigh.push_back(g_fxMan.CreateFxSystem(bpHigh, mat, vehMat, true));
                data.m_matrices.push_back(mat);
            }
        }
    }
}

void BackFireEffect::BackFireSingle(CVehicle *pVeh, bool bPlaySound)
{
    BackfireData &data = m_VehData.Get(pVeh);
    EnsureSystemsCreated(pVeh, data);

    size_t totalPipes = data.m_fxLow.size();
    if (totalPipes == 0)
    {
        return;
    }

    // VEHFUNCS REALISTIC DYNAMICS:
    // Most shifts (75%): Soft crackling puffs with independent random sizes (e.g. left tiny, right medium).
    // Rare hard shifts (25%): One pipe gets a high-intensity blast, while the other maintains a subtle crackle.
    bool isHighBlastEvent = (rand() % 100) < 25;
    size_t highPipeIdx = rand() % totalPipes;

    for (size_t i = 0; i < totalPipes; i++)
    {
        if (isHighBlastEvent && i == highPipeIdx)
        {
            if (data.m_fxHigh[i])
            {
                data.m_fxHigh[i]->SetRateMult(CGeneral::GetRandomNumberInRange(0.75f, 0.95f));
                data.m_fxHigh[i]->Play();
            }
        }
        else
        {
            if (data.m_fxLow[i])
            {
                // Independent roll between 0.35f and 0.55f: one is smaller (e.g. 0.36), one is medium (e.g. 0.52), but NEVER 0!
                data.m_fxLow[i]->SetRateMult(CGeneral::GetRandomNumberInRange(0.35f, 0.55f));
                data.m_fxLow[i]->Play();
            }
        }
    }

    if (bPlaySound)
    {
        CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
        CVector pos = (pInfo && pInfo->m_pVehicleStruct) ? pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST] : CVector(0.0f, -2.0f, 0.0f);
        CVector exhaustWorldPos = pVeh->TransformFromObjectSpace(pos);
        static std::string audioPath = MOD_DATA_PATH("audio/backfire.wav");
        AudioMgr::Play3DSound(audioPath, exhaustWorldPos, pVeh, 1.5f, 80.0f);
    }
}

void BackFireEffect::BackFireMulti(CVehicle *pVeh, bool bPlaySound)
{
    int num = RandomNumberInRange(0, 3) - 1;

    BackFireSingle(pVeh, bPlaySound);
    BackfireData &data = m_VehData.Get(pVeh);
    if (num > 0)
    {
        data.m_nleftFires = num;
    }
    else
    {
        data.m_nleftFires = 0;
    }
}

std::vector<int> ValidModels = {};
bool onlySelected = false;

void BackFireEffect::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    std::string line = gConfig.ReadString("TABLE", "BackFireEffect_VehicleModels", "");
    onlySelected = gConfig.ReadBoolean("FEATURES", "BackfireEffect_OnlySelectedModels", true);
    ValidModels.clear();
    Util::GetModelsFromIni(line, ValidModels);
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

// Inspired by Junior's https://www.mixmods.com.br/2016/06/backfire-als-v2-5-mod-estalar-escapamento/
void BackFireEffect::Process(CVehicle *pVeh)
{
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::BackfireEffect))
    {
        return;
    }

    if (!pVeh->GetIsOnScreen() || pVeh->bEngineBroken || !pVeh->bEngineOn || pVeh->bIsBig || pVeh->bIsVan || pVeh->bIsBus || pVeh->bIsRCVehicle)
    {
        return;
    }

    if (pVeh->m_nVehicleSubClass != VEHICLE_BIKE && pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE)
    {
        return;
    }

    bool isValidVeh = std::find(ValidModels.begin(), ValidModels.end(), pVeh->m_nModelIndex) != ValidModels.end();

    // 1. Check Soundize compatibility
    SoundizeAPI::Init();
    bool bUsingSoundize = SoundizeAPI::IsPresent() && SoundizeAPI::fnIsVehicleUsingAnyBank && (SoundizeAPI::fnIsVehicleUsingAnyBank(pVeh) != 0);
    bool bSoundizeSupportsBF = bUsingSoundize && SoundizeAPI::fnDoesVehicleSupportBackfire && (SoundizeAPI::fnDoesVehicleSupportBackfire(pVeh) != 0);

    // Smart validation: If Soundize explicitly supports backfire for this vehicle, auto-validate it!
    if (bSoundizeSupportsBF)
    {
        isValidVeh = true;
    }

    if (!isValidVeh && onlySelected)
    {
        return;
    }

    BackfireData &data = m_VehData.Get(pVeh);

    if (bUsingSoundize)
    {
        // Soundize FMOD gear shift detection:
        // Soundize shifts gear based on FMOD engine RPM (RPM gear) and cuts gas pedal.
        // This brings the authentic gear shift flame transition to Soundize-adapted cars!
        if (SoundizeAPI::fnGetVehicleGear)
        {
            int szGear = SoundizeAPI::fnGetVehicleGear(pVeh);
            if (szGear <= 1)
            {
                data.lastSoundizeGear = 0; // Reverse (0) or Neutral (1)
            }
            else if (data.lastSoundizeGear >= 2 && szGear > data.lastSoundizeGear)
            {
                float speed = Util::GetVehicleSpeed(pVeh);
                if (pVeh->m_pDriver && speed > 3.0f)
                {
                    // Only mute ModelExtras's WAV sound if Soundize provides backfire sound for this vehicle!
                    BackFireSingle(pVeh, /*bPlaySound=*/ !bSoundizeSupportsBF);
                }
                data.lastSoundizeGear = szGear;
            }
            else
            {
                data.lastSoundizeGear = szGear;
            }
        }

        // Soundize ALS backfire (on throttle release):
        if (SoundizeAPI::fnGetVehicleDoingBackfire)
        {
            int doingBF = SoundizeAPI::fnGetVehicleDoingBackfire(pVeh);
            if (doingBF > 0)
            {
                // If VehFuncs is not installed, ModelExtras provides the visual flame on DFF exhausts
                if (!VehFuncsCompat::IsPresent())
                {
                    BackFireSingle(pVeh, /*bPlaySound=*/ false);
                }
            }
        }
        return;
    }

    // 2. Vanilla / non-Soundize vehicle branch:
    // Uses vanilla audio loop reset (0x284 == 0) and engine RPM (0x280) for the perfectly-timed
    // gear shift and suspension squat sync.
    unsigned short rpm = *(unsigned short *)((int)pVeh + 0x280);
    unsigned char gchanging = *(unsigned char *)((int)pVeh + 0x284);
    unsigned char nitroActivated = *(unsigned char *)((int)pVeh + 0x37C);
    float speed = Util::GetVehicleSpeed(pVeh);

    float throttle = pVeh->m_fGasPedal;
    if (throttle < 0.0f)
    {
        throttle = -throttle;
    }

    if (pVeh->m_nCurrentGear > 1 && pVeh->m_pDriver && gchanging == 0 && rpm != 65535 && rpm > 100.0f && speed > 3.0f)
    {
        BackFireSingle(pVeh, /*bPlaySound=*/ true);
    }

    // Throttle lift-off / ALS multi-fire logic
    // Realistic automotive conditions: Only triggers when lifting off from high RPM at driving speed.
    // Prevents fake backfires when stationary, holding brakes, or revving at idle.
    size_t timer = CTimer::m_snTimeInMilliseconds;

    if (data.wasFullThrottled)
    {
        if (throttle < 0.78f)
        {
            if (speed > 5.0f)
            {
                BackFireMulti(pVeh, /*bPlaySound=*/ true);
            }
            data.wasFullThrottled = false;
        }
    }
    else
    {
        if (speed > 5.0f && (throttle >= 0.99f || (throttle > 0.39f && nitroActivated)))
        {
            data.wasFullThrottled = true;
        }
    }

    if (timer - data.prevTimer > 200)
    {
        if (data.m_nleftFires > 0)
        {
            if (speed > 3.0f)
            {
                BackFireSingle(pVeh, /*bPlaySound=*/ true);
            }
            data.m_nleftFires--;
        }
        data.prevTimer = timer;
    }
}