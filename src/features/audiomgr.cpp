#include "pch.h"
#include "audiomgr.h"
#include "defines.h"
#include <extensions/ScriptCommands.h>
#include <CAudioEngine.h>

#define LOAD_AUDIO_STREAM 0x0AAC
#define LOAD_3D_AUDIO_STREAM 0x0AC1
#define SET_PLAY_3D_AUDIO_STREAM_AT_COORDS 0x0AC2
#define SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT 0x0AC3
#define SET_PLAY_3D_AUDIO_STREAM_AT_CHAR 0x0AC4
#define SET_PLAY_3D_AUDIO_STREAM_AT_CAR 0x0AC5
#define SET_AUDIO_STREAM_STATE 0x0AAD
#define GET_AUDIO_STREAM_STATE 0x0AB9
#define REMOVE_AUDIO_STREAM 0x0AAE
#define SET_AUDIO_STREAM_VOLUME 0x0ABC

void AudioMgr::Initialize()
{
    plugin::Events::reInitGameEvent += []
    {
        m_NeedToFree.clear();

        for (auto &e : m_Cache)
        {
            e.second = Load(e.first);
        }
    };

    plugin::Events::processScriptsEvent += []
    {
        static size_t prev = 0;
        size_t cur = CTimer::m_snTimeInMilliseconds;

        if (cur - prev > 5000)
        {
            for (auto it = m_NeedToFree.begin(); it != m_NeedToFree.end();)
            {
                if (!*it)
                {
                    continue;
                }

                int state = eAudioStreamState::Stopped;
                plugin::Command<GET_AUDIO_STREAM_STATE>(*it, &state);
                if (state == eAudioStreamState::Stopped || state == eAudioStreamState::Paused)
                {
                    plugin::Command<REMOVE_AUDIO_STREAM>(*it);
                    *it = NULL;
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

void AudioMgr::PlaySwitchSound(CEntity *pEntity)
{
    static std::string path = MOD_DATA_PATH("audio/effects/switch_toggle.wav");
    PlayFileSound(path, pEntity, 1.0f, true);
}

std::unordered_map<std::string, bool> is3DSupported;

StreamHandle AudioMgr::Load(const std::string &path)
{
    if (path.empty())
    {
        return false;
    }

    StreamHandle handle = NULL;
    if (!is3DSupported.contains(path) || is3DSupported[path]) {
        plugin::Command<LOAD_3D_AUDIO_STREAM>(path.c_str(), &handle);
    }
    
    if (handle) {
        is3DSupported[path] = true;
    } else {
        plugin::Command<LOAD_AUDIO_STREAM>(path.c_str(), &handle);
        if (handle) {
            is3DSupported[path] = false;
        } else {
            LOG_VERBOSE("Failed to load sound '{}'", path);
            return NULL;
        }
    }
    return handle;
}

void AudioMgr::PlayFileSound(const std::string &path, CEntity *pEntity, float volume, bool cached)
{
    StreamHandle handle = NULL;

    if (cached)
    {
        if (!m_Cache.contains(path))
        {
            StreamHandle temp = Load(path);
            if (temp)
            {
                m_Cache[path] = temp;
            }
            else
            {
                return;
            }
        }
        handle = m_Cache[path];
    }
    else
    {
        handle = Load(path);
        if (handle) {
            m_NeedToFree.push_back(handle);
        }
    }

    if (!handle)
    {
        return;
    }

    int state = eAudioStreamState::Stopped;
    plugin::Command<GET_AUDIO_STREAM_STATE>(handle, &state);

    if (state != eAudioStreamState::Playing)
    {
        SetVolume(handle, volume);
        if (!pEntity)
        {
            pEntity = FindPlayerPed();
        }

        if (pEntity->m_nType == ENTITY_TYPE_VEHICLE)
        {
            int hEntity = CPools::GetVehicleRef(static_cast<CVehicle *>(pEntity));
            if (hEntity)
            {
                plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_CAR>(handle, hEntity);
            }
        }
        else if (pEntity->m_nType == ENTITY_TYPE_PED)
        {
            int hEntity = CPools::GetPedRef(static_cast<CPed *>(pEntity));
            if (hEntity)
            {
                plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_CHAR>(handle, hEntity);
            }
        }
        else if (pEntity->m_nType == ENTITY_TYPE_OBJECT)
        {
            int hEntity = CPools::GetObjectRef(static_cast<CObject *>(pEntity));
            if (hEntity)
            {
                plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT>(handle, hEntity);
            }
        }
        else
        {
            CVector pos = pEntity->GetPosition();
            plugin::Command<SET_PLAY_3D_AUDIO_STREAM_AT_COORDS>(handle, pos.x, pos.y, pos.z);
        }
        plugin::Command<SET_AUDIO_STREAM_STATE>(handle, static_cast<int>(eAudioStreamState::Playing));
    }
}

void AudioMgr::SetVolume(StreamHandle handle, float volume) {
    if (handle) {
        plugin::Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f * volume);
    }
}