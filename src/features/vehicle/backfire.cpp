#include "pch.h"
#include "defines.h"
#include "backfire.h"
#include "datamgr.h"
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include "../audiomgr.h"
#include "../../enums/vehdummy.h"
#include <CCamera.h>

void BackFireEffect::BackFireFX(CVehicle *pVeh, float x, float y, float z)
{
    int handle = NULL;
    plugin::Command<Commands::CREATE_FX_SYSTEM_ON_CAR_WITH_DIRECTION>("GUNFLASH", CPools::GetVehicleRef(pVeh), x, y, z, 0.0f, -25.0f, 0.0f, 1, &handle);

    if (handle == NULL)
    {
        return;
    }

    plugin::Command<Commands::PLAY_AND_KILL_FX_SYSTEM>(handle);

    CVector vehPos = pVeh->GetPosition();
    CVector camPos = TheCamera.GetPosition();
    static std::string audioPath = MOD_DATA_PATH("audio/effects/backfire.wav");
    AudioMgr::PlayFileSound(audioPath, pVeh, 1.5f, true);
    // if (DistanceBetweenPoints(vehPos, camPos) < 80.0f)
    // {
    //     plugin::Command<Commands::ADD_ONE_OFF_SOUND>(0.0f, 0.0f, 0.0f, 1131);
    // }
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

    if (pHandlingData->m_bDoubleExhaust) {
        BackFireFX(pVeh, vx, pos.y, pos.z);
    }
    BackFireFX(pVeh, pos.x, pos.y, pos.z);

    vx = 0.0f;
    pos = pInfo->m_pVehicleStruct->m_avDummyPos[eVehicleDummies::EXHAUST_SECONDARY];
    if (!pos.IsZero()) {
        if (pHandlingData->m_bDoubleExhaust)
        {
            vx = pos.x * -1.0f;
        }

        if (pHandlingData->m_bDoubleExhaust) {
            BackFireFX(pVeh, vx, pos.y, pos.z);
        }
        BackFireFX(pVeh, pos.x, pos.y, pos.z);
    }
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

std::vector<int> ValidModels = {};
bool onlySelected = false;

void BackFireEffect::Initialize(RwFrame *frame, CEntity *pVeh)
{
    plugin::Events::initGameEvent += []()
    {
        std::string line = gConfig.ReadString("TABLE", "BackFireEffect_VehicleModels", "");
        onlySelected = gConfig.ReadBoolean("VEHICLE_FEATURES", "BackfireEffect_OnlySelectedModels", true);
        Util::GetModelsFromIni(line, ValidModels);
    };

    plugin::Events::vehicleRenderEvent.before += [](CVehicle *vehicle)
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

    if (pVeh->m_nCurrentGear == 0) {
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
        static size_t prevTimer = 0;
        size_t timer = CTimer::m_snTimeInMilliseconds;

        if (data.wasFullThrottled)
        {
            if (acclPadel < 100)
            {
                BackFireMulti(pVeh);
                data.wasFullThrottled = false;
            }
        }
        else
        {
            if (acclPadel == 128 || (acclPadel > 50 && nitroActivated))
            {
                data.wasFullThrottled = true;
            }
        }

        if (timer - prevTimer > 200)
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