#include "pch.h"
#include "backfire.h"
#include "datamgr.h"
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include "../audiomgr.h"
#include "../../enums/vehdummy.h"

static bool flag = false;

void BackFireEffect::BackFireFX(CVehicle *pVeh, float x, float y, float z)
{
    static int handle = NULL;

    if (handle == NULL)
    {
        plugin::Command<Commands::CREATE_FX_SYSTEM_ON_CAR_WITH_DIRECTION>("GUNFLASH", CPools::GetVehicleRef(pVeh), x, y, z, 0.0f, -25.0f, 0.0f, 1, &handle);
    }
    plugin::Command<Commands::PLAY_FX_SYSTEM>(handle);

    static std::string path = MOD_DATA_PATH("audio/effects/backfire.wav");
    static StreamHandle hAudio = NULL;
    if (hAudio == NULL)
    {
        hAudio = AudioMgr::Load(&path);
    }
    AudioMgr::Play(hAudio, pVeh);
}

void BackFireEffect::BackFireSingle(CVehicle *pVeh)
{
    // https://github.com/multitheftauto/mtasa-blue/blob/16769b8d1c94e2b9fe6323dcba46d1305f87a190/Client/game_sa/CModelInfoSA.h#L213
    CVehicleModelInfo *pInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
    int handlingID = patch::Get<WORD>((int)pInfo + 74, false);                                       //  CBaseModelInfo + 74 = handlingID
    tHandlingData *pHandlingData = reinterpret_cast<tHandlingData *>(0xC2B9DC + (handlingID * 224)); // sizeof(tHandlingData) = 224
    float vx = 0;
    CVector pos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST];
    if (pHandlingData->m_bDoubleExhaust)
    {
        vx = pos.x * -1.0f;
    }

    if (pHandlingData->m_bDoubleExhaust)
    {
        BackFireFX(pVeh, vx, pos.y, pos.z);
    }
    BackFireFX(pVeh, pos.x, pos.y, pos.z);
}

void BackFireEffect::BackFireMulti(CVehicle *pVeh)
{
    int num = plugin::RandomNumberInRange(0, 3) - 1;

    BackFireSingle(pVeh);
    VehData &data = vehData.Get(pVeh);
    if (num > 0)
    {
        data.m_nleftFires = num;
    }
    else
    {
        data.m_nleftFires = 0;
    }
}

// Inspired by Junior's https://www.mixmods.com.br/2016/06/backfire-als-v2-5-mod-estalar-escapamento/
void BackFireEffect::Process(void *ptr, RwFrame *frame, eModelEntityType type)
{
    CVehicle *pVeh = static_cast<CVehicle *>(ptr);

    if (!pVeh->GetIsOnScreen() || pVeh->m_nVehicleFlags.bIsBig || pVeh->m_nVehicleFlags.bIsVan || pVeh->m_nVehicleFlags.bIsBus || pVeh->m_nVehicleFlags.bIsRCVehicle)
    {
        return;
    }
    VehData &data = vehData.Get(pVeh);

    if (pVeh->m_nVehicleSubClass == VEHICLE_BIKE || pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
    {
        unsigned short rpm = *(unsigned short *)((int)pVeh + 0x280);
        unsigned char gchanging = *(unsigned char *)((int)pVeh + 0x284);
        unsigned char nitroActivated = *(unsigned char *)((int)pVeh + 0x37C);
        float speed = Util::GetVehicleSpeed(pVeh);
        unsigned char acclPadel = *(unsigned char *)((int)pVeh + 0x966);

        if (pVeh->m_pDriver && gchanging == 0 && rpm != 65535 && rpm > 100.0f && speed > 5.0f)
        {
            BackFireSingle(pVeh);
        }

        // handle multi
        static size_t prevTimer = 0, prevTimer2 = 0;
        size_t timer = CTimer::m_snTimeInMilliseconds;

        if (timer - prevTimer2 > 100)
        {
            if (flag)
            {
                if (acclPadel < 100)
                {
                    BackFireMulti(pVeh);
                    flag = false;
                }
            }
            else
            {
                if (acclPadel == 128)
                {
                    if (pVeh->m_nVehicleFlags.bEngineOn)
                    {
                        BackFireMulti(pVeh);
                    }
                    flag = true;
                }
            }
            prevTimer2 = timer;
        }

        if (timer - prevTimer > 50)
        {
            if (data.m_nleftFires > 0)
            {
                BackFireSingle(pVeh);
                data.m_nleftFires--;
            }
            prevTimer = timer;
        }
    }
}