#include "pch.h"
#include "audiomgr.h"
#include <extensions/ScriptCommands.h>
#include <CAudioEngine.h>

#define LOAD_AUDIO_STREAM 0x0AAC
#define SET_PLAY_3D_AUDIO_STREAM_AT_COORDS 0x0AC2
#define SET_PLAY_3D_AUDIO_STREAM_AT_CHAR 0x0AC4
#define SET_PLAY_3D_AUDIO_STREAM_AT_CAR 0x0AC5
#define SET_AUDIO_STREAM_STATE 0x0AAD
#define GET_AUDIO_STREAM_STATE 0x0AB9
#define REMOVE_AUDIO_STREAM 0x0AAE
#define SET_AUDIO_STREAM_VOLUME 0x0ABC

void AudioMgr::Initialize()
{
    plugin::Events::processScriptsEvent += []
    {
        static size_t prev = 0;
        size_t cur = CTimer::m_snTimeInMilliseconds;

        if (cur - prev > 5000)
        {
            for (auto it = m_NeedToFree.begin(); it != m_NeedToFree.end();)
            {
                int state = eAudioStreamState::Stopped;
                plugin::Command<GET_AUDIO_STREAM_STATE>(*it, &state);
                if (state == eAudioStreamState::Stopped || state == eAudioStreamState::Paused)
                {
                    plugin::Command<REMOVE_AUDIO_STREAM>(*it);
                    it = m_NeedToFree.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            prev = cur;
        };
    };
}

void AudioMgr::PlayClickSound()
{
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON, 10.0, 1.0);
}

StreamHandle AudioMgr::Load(std::string *pPath)
{
    if (!pPath)
    {
        return NULL;
    }

    StreamHandle handle = NULL;
    plugin::Command<LOAD_AUDIO_STREAM>(pPath->c_str(), &handle);
    if (!handle)
    {
        LOG_VERBOSE("Failed to load sound '{}'", *pPath);
    }
    return handle;
}

void AudioMgr::SetVolume(StreamHandle handle, float volume)
{
    plugin::Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f * volume);
}

void AudioMgr::Play(StreamHandle handle, CEntity *pEntity, float volume)
{
    if (!handle)
    {
        return;
    }

    int state = eAudioStreamState::Stopped;
    plugin::Command<GET_AUDIO_STREAM_STATE>(handle, &state);

    if (state != eAudioStreamState::Playing)
    {
        CVector pos = pEntity->GetPosition();
        plugin::Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f * volume);
        plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_COORDS>(handle, pos.x, pos.y, pos.z);
        plugin::Command<SET_AUDIO_STREAM_STATE>(handle, static_cast<int>(eAudioStreamState::Playing));
    }
}

void AudioMgr::PlayOnVehicle(StreamHandle handle, CVehicle *pVeh, float volume)
{
    if (!handle)
    {
        return;
    }

    int state = eAudioStreamState::Stopped;
    plugin::Command<GET_AUDIO_STREAM_STATE>(handle, &state);

    if (state != eAudioStreamState::Playing)
    {
        plugin::Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f * volume);
        plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_CAR>(CPools::GetVehicleRef(pVeh));
        plugin::Command<SET_AUDIO_STREAM_STATE>(handle, static_cast<int>(eAudioStreamState::Playing));
    }
}

void AudioMgr::LoadAndPlayOnVehicle(std::string *pPath, CVehicle *pVeh)
{
    if (!pPath || !pVeh)
    {
        return;
    }

    StreamHandle handle = NULL;
    plugin::Command<LOAD_AUDIO_STREAM>(pPath->c_str(), &handle);
    if (!handle)
    {
        LOG_VERBOSE("Failed to load sound '{}'", *pPath);
    }
    plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_CAR>(CPools::GetVehicleRef(pVeh));
    plugin::Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f);
    plugin::Command<SET_AUDIO_STREAM_STATE>(handle, static_cast<int>(eAudioStreamState::Playing));
    m_NeedToFree.push_back(handle);
}

void AudioMgr::LoadAndPlay(std::string *pPath, CEntity *pEntity)
{
    if (!pPath || !pEntity)
    {
        return;
    }

    CVector pos = pEntity->GetPosition();
    StreamHandle handle = NULL;
    plugin::Command<LOAD_AUDIO_STREAM>(pPath->c_str(), &handle);
    if (!handle)
    {
        LOG_VERBOSE("Failed to load sound '{}'", *pPath);
    }
    plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_COORDS>(handle, pos.x, pos.y, pos.z);
    plugin::Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f);
    plugin::Command<SET_AUDIO_STREAM_STATE>(handle, static_cast<int>(eAudioStreamState::Playing));
    m_NeedToFree.push_back(handle);
}