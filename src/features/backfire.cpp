#include "pch.h"
#include "defines.h"
#include "backfire.h"
#include "utils/datamgr.h"
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include "utils/audiomgr.h"
#include "enums/vehdummy.h"
#include <CCamera.h>
#include "ModelExtrasAPI.h"

using namespace plugin;

void BackFireEffect::BackFireFX(CVehicle *pVeh, float x, float y, float z, float dirX, float dirY, float dirZ)
{
    int handle = NULL;
    Command<Commands::CREATE_FX_SYSTEM_ON_CAR_WITH_DIRECTION>("GUNFLASH", CPools::GetVehicleRef(pVeh), x, y, z, dirX, dirY, dirZ, 1, &handle);

    if (handle == NULL)
    {
        return;
    }

    Command<Commands::PLAY_AND_KILL_FX_SYSTEM>(handle);

    CVector vehPos = pVeh->GetPosition();
    CVector camPos = TheCamera.GetPosition();
    static std::string audioPath = MOD_DATA_PATH("audio/backfire.wav");
    AudioMgr::PlayFileSound(audioPath, pVeh, 1.5f, true);
}

void BackFireEffect::BackFireSingle(CVehicle *pVeh)
{

    size_t count = ME_GetExhaustCount(pVeh);
    if (count <= 0)
    {
        // https://github.com/multitheftauto/mtasa-blue/blob/16769b8d1c94e2b9fe6323dcba46d1305f87a190/Client/game_sa/CModelInfoSA.h#L213
        CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
        float vx = 0;
        CVector pos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST];
        if (pVeh->m_pHandlingData->m_bDoubleExhaust)
        {
            vx = pos.x * -1.0f;
        }

        if (pVeh->m_pHandlingData->m_bDoubleExhaust)
        {
            BackFireFX(pVeh, vx, pos.y, pos.z);
        }
        BackFireFX(pVeh, pos.x, pos.y, pos.z);

        vx = 0.0f;
        pos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST_SECONDARY];
        if (!pos.IsZero())
        {
            if (pVeh->m_pHandlingData->m_bDoubleExhaust)
            {
                vx = pos.x * -1.0f;
            }

            if (pVeh->m_pHandlingData->m_bDoubleExhaust)
            {
                BackFireFX(pVeh, vx, pos.y, pos.z);
            }
            BackFireFX(pVeh, pos.x, pos.y, pos.z);
        }
    }
    else
    {
        for (size_t i = 0; i < count; i++)
        {
            const ME_ExhaustInfo &info = ME_GetExhaustData(pVeh, static_cast<int>(i));
            if (info.pFrame)
            {
                CVector f = info.pFrame->modelling.up; // Up is Forward
                BackFireFX(pVeh, info.pFrame->modelling.pos.x, info.pFrame->modelling.pos.y, info.pFrame->modelling.pos.z, f.x * 1.5f, f.y * 1.5f, f.z * 1.5f);
            }
        }
    }
}

void BackFireEffect::BackFireMulti(CVehicle *pVeh)
{
    int num = RandomNumberInRange(0, 3) - 1;

    BackFireSingle(pVeh);
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

void BackFireEffect::Init()
{
    Events::initGameEvent += []()
    {
        std::string line = gConfig.ReadString("TABLE", "BackFireEffect_VehicleModels", "");
        onlySelected = gConfig.ReadBoolean("FEATURES", "BackfireEffect_OnlySelectedModels", true);
        Util::GetModelsFromIni(line, ValidModels);
    };

    Events::vehicleRenderEvent.before += [](CVehicle *vehicle)
    {
        BackFireEffect::Process(vehicle);
    };
}

// Inspired by Junior's https://www.mixmods.com.br/2016/06/backfire-als-v2-5-mod-estalar-escapamento/
void BackFireEffect::Process(CVehicle *pVeh)
{
    if (!pVeh->GetIsOnScreen() || pVeh->bEngineBroken || !pVeh->bEngineOn || pVeh->bIsBig || pVeh->bIsVan || pVeh->bIsBus || pVeh->bIsRCVehicle)
    {
        return;
    }

    bool isValidVeh = std::find(ValidModels.begin(), ValidModels.end(), pVeh->m_nModelIndex) != ValidModels.end();

    if (!isValidVeh && onlySelected)
    {
        return;
    }

    if (pVeh->m_nCurrentGear == 0)
    {
        return;
    }

    BackfireData &data = m_VehData.Get(pVeh);

    if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE || pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
    {
        unsigned short rpm = *(unsigned short *)((int)pVeh + 0x280);
        unsigned char gchanging = *(unsigned char *)((int)pVeh + 0x284);
        unsigned char nitroActivated = *(unsigned char *)((int)pVeh + 0x37C);
        float speed = Util::GetVehicleSpeed(pVeh);

        // sizeof(CBike) is 0x814, so reading pVeh + 0x966 ran past the end of the
        // object on every bike and fed the throttle checks below whatever happened to
        // sit in the heap after it. It only stayed in bounds because sizeof(CAutomobile)
        // is 0x988, which is why this looked like it worked on cars. m_fGasPedal is a
        // CVehicle member, so it is valid for both classes.
        float throttle = pVeh->m_fGasPedal;
        if (throttle < 0.0f)
        {
            throttle = -throttle;
        }

        if (pVeh->m_pDriver && gchanging == 0 && rpm != 65535 && rpm > 100.0f && speed > 5.0f)
        {
            BackFireSingle(pVeh);
        }

        // handle multi
        size_t timer = CTimer::m_snTimeInMilliseconds;

        if (data.wasFullThrottled)
        {
            if (throttle < 0.78f)
            {
                BackFireMulti(pVeh);
                data.wasFullThrottled = false;
            }
        }
        else
        {
            if (throttle >= 0.99f || (throttle > 0.39f && nitroActivated))
            {
                data.wasFullThrottled = true;
            }
        }

        if (timer - data.prevTimer > 200)
        {
            if (data.m_nleftFires > 0)
            {
                BackFireSingle(pVeh);
                data.m_nleftFires--;
            }
            data.prevTimer = timer;
        }
    }
}