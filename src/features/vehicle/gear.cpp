#include "pch.h"
#include "Gear.h"
#include "bass.h"
#include "datamgr.h"
#include <extensions/ScriptCommands.h>

#define LOAD_3D_AUDIO_STREAM 0x0AC1
#define SET_PLAY_3D_AUDIO_STREAM_AT_COORDS 0x0AC2
#define IS_AUDIO_STREAM_PLAYING 0x2500
#define SET_AUDIO_STREAM_PROGRESS 0x2508
#define SET_AUDIO_STREAM_TYPE 0x250A
#define SET_AUDIO_STREAM_STATE 0x0AAD

void Clutch::Process(void *ptr, RwFrame *frame, eModelEntityType type)
{
    CVehicle *pVeh = static_cast<CVehicle *>(ptr);
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);

    if (!data.m_bInitialized)
    {
        data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        if (jsonData.contains("Clutch") && jsonData["Clutch"].contains("Offset"))
        {
            data.m_nCurOffset = jsonData["Clutch"].value("Offset", 0.0f);
        }
        data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
        data.m_bInitialized = true;
    }

    uint timer = CTimer::m_snTimeInMilliseconds;
    uint deltaTime = (timer - data.m_nLastFrameMS);

    if (deltaTime > data.m_nWaitTime)
    {
        if (data.m_eState == eFrameState::AtOrigin)
        {
            if (pVeh->m_nCurrentGear != data.m_nLastGear)
            {
                data.m_nLastGear = pVeh->m_nCurrentGear;
                data.m_eState = eFrameState::IsMoving;
            }
        }
        else
        {
            if (data.m_eState == eFrameState::AtOffset)
            {
                if (data.m_fCurRotation == 0)
                {
                    data.m_eState = eFrameState::AtOrigin;
                }
                else
                {
                    if (data.m_nCurOffset > 0)
                    {
                        data.m_fCurRotation -= 1;
                        data.m_fCalVal = -1;
                    }
                    else
                    {
                        data.m_fCurRotation += 1;
                        data.m_fCalVal = 1;
                    }

                    Util::SetFrameRotationZ(frame, data.m_fCalVal);
                    data.m_nLastFrameMS = timer;
                }
            }
            else
            {
                if (data.m_fCurRotation == data.m_nCurOffset)
                {
                    data.m_eState = eFrameState::AtOffset;
                }
                else
                {
                    if (data.m_nCurOffset < data.m_fCurRotation)
                    {
                        data.m_fCurRotation -= 1;
                        data.m_fCalVal = -1;
                    }
                    else
                    {
                        data.m_fCurRotation += 1;
                        data.m_fCalVal = 1;
                    }

                    Util::SetFrameRotationZ(frame, data.m_fCalVal);
                    data.m_nLastFrameMS = timer;
                }
            }
        }
    }
}

void GearLever::Process(void *ptr, RwFrame *frame, eModelEntityType type)
{
    CVehicle *pVeh = static_cast<CVehicle *>(ptr);
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(frame);

    if (!data.m_bInitialized)
    {
        data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_(-?[0-9]+).*", "0"));
        if (data.m_nCurOffset == 0)
        {
            data.m_nCurOffset = std::stoi(Util::GetRegexVal(name, ".*_o(-?[0-9]+).*", "0"));
        }
        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        if (jsonData.contains("GearLever") && jsonData["GearLever"].contains("Offset"))
        {
            data.m_nCurOffset = jsonData["GearLever"].value("Offset", 0.0f);
        }
        data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nCurOffset / 10));
        data.m_bInitialized = true;
    }

    uint timer = CTimer::m_snTimeInMilliseconds;
    uint deltaTime = (timer - data.m_nLastFrameMS);

    if (deltaTime > data.m_nWaitTime)
    {
        if (data.m_eState == eFrameState::AtOrigin)
        {
            if (pVeh->m_nCurrentGear != data.m_nLastGear)
            {
                data.m_fCalVal = (pVeh->m_nCurrentGear > data.m_nLastGear) ? -1.0f : 1.0f;
                data.m_nLastGear = pVeh->m_nCurrentGear;
                data.m_eState = eFrameState::IsMoving;
            }
        }
        else
        {
            if (data.m_eState == eFrameState::AtOffset)
            {
                if (data.m_fCurRotation != 0)
                {
                    if (data.m_fCurRotation > 0)
                    {
                        data.m_fCurRotation -= 1;
                        data.m_fCalVal = -1;
                    }
                    else
                    {
                        data.m_fCurRotation += 1;
                        data.m_fCalVal = 1;
                    }
                    Util::SetFrameRotationX(frame, data.m_fCalVal);
                }
                else
                {
                    data.m_eState = eFrameState::AtOrigin;
                }
            }
            else
            {
                if (data.m_nCurOffset != abs(data.m_fCurRotation))
                {
                    data.m_fCurRotation += data.m_fCalVal;
                    Util::SetFrameRotationX(frame, data.m_fCalVal);
                }
                else
                {
                    data.m_eState = eFrameState::AtOffset;
                }
            }

            data.m_nLastFrameMS = timer;
        }
    }
}

// TODO: Redo this feature
void GearSound::Process(void *ptr, RwFrame *frame, eModelEntityType type)
{
    CVehicle *pVeh = static_cast<CVehicle *>(ptr);
    std::string name = GetFrameNodeName(frame);
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized)
    {
        std::string fileName = Util::GetRegexVal(name, "x_gs_(.*$)", "");
        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        if (jsonData.contains("GearSound") && jsonData["GearSound"].contains("Name"))
        {
            fileName = jsonData["GearSound"].value("Name", 0.0f);
        }
        std::string upPath = MOD_DATA_PATH_S(std::format("audio/gear/{}.wav", fileName));
        plugin::Command<LOAD_3D_AUDIO_STREAM>(upPath.c_str(), &data.hUpAudio);
        data.m_bInitialized = true;
    }
    if (data.m_nCurGear != pVeh->m_nCurrentGear)
    {
        if (data.hUpAudio && !plugin::Command<IS_AUDIO_STREAM_PLAYING>(data.hUpAudio))
        {
            plugin::Command<SET_AUDIO_STREAM_PROGRESS>(data.hUpAudio, 0.0f);
            CVector pos = pVeh->GetPosition();
            plugin::Command<SET_AUDIO_STREAM_TYPE>(data.hUpAudio, 1);
            plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_COORDS>(data.hUpAudio, pos.x, pos.y, pos.z);
            plugin::Command<SET_AUDIO_STREAM_STATE>(data.hUpAudio, 1);
        }
        data.m_nCurGear = pVeh->m_nCurrentGear;
    }
}