#include "pch.h"
#include "audiomgr.h"
#include "defines.h"
#include <cstdint>
#include <extensions/ScriptCommands.h>
#include <CAudioEngine.h>

using namespace plugin;

enum : uint16_t {
    LOAD_AUDIO_STREAM = 0x0AAC,
    LOAD_3D_AUDIO_STREAM = 0x0AC1,
    SET_PLAY_3D_AUDIO_STREAM_AT_COORDS = 0x0AC2,
    SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT = 0x0AC3,
    SET_PLAY_3D_AUDIO_STREAM_AT_CHAR = 0x0AC4,
    SET_PLAY_3D_AUDIO_STREAM_AT_CAR = 0x0AC5,
    SET_AUDIO_STREAM_STATE = 0x0AAD,
    GET_AUDIO_STREAM_STATE = 0x0AB9,
    REMOVE_AUDIO_STREAM = 0x0AAE,
    SET_AUDIO_STREAM_VOLUME = 0x0ABC
};

void AudioMgr::Initialize()
{
    Events::reInitGameEvent += []
    {
        needToFree.clear();

        for (auto &pEnt : cache)
        {
            pEnt.second = Load(pEnt.first);
        }
    };

    Events::processScriptsEvent += []
    {
        static size_t prev = 0;
        size_t cur = CTimer::m_snTimeInMilliseconds;

        if (cur - prev > 5000)
        {
            for (auto it = needToFree.begin(); it != needToFree.end();)
            {
                if (!*it)
                {
                    continue;
                }

                int state = eAudioStreamState::Stopped;
                Command<GET_AUDIO_STREAM_STATE>(*it, &state);
                if (state == eAudioStreamState::Stopped || state == eAudioStreamState::Paused)
                {
                    Command<REMOVE_AUDIO_STREAM>(*it);
                    *it = NULL;
                    it = needToFree.erase(it);
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
    if (!ShouldPlaySound())
    {
        return;
    }
    AudioEngine.ReportFrontendAudioEvent(AE_FRONTEND_RADIO_CLICK_ON, 10.0, 1.0);
}

void AudioMgr::PlaySwitchSound(CEntity *pEntity)
{
    if (!ShouldPlaySound())
    {
        return;
    }
    static std::string path = MOD_DATA_PATH("audio/effects/switch_toggle.wav");
    PlayFileSound(path, pEntity, 1.0f, true);
}

std::unordered_map<std::string, bool> is3DSupported;

StreamHandle AudioMgr::Load(const std::string &path)
{
    if (path.empty())
    {
        return NULL;
    }

    StreamHandle handle = NULL;
    if (!is3DSupported.contains(path) || is3DSupported[path])
    {
        Command<LOAD_3D_AUDIO_STREAM>(path.c_str(), &handle);
    }

    if (handle != NULL)
    {
        is3DSupported[path] = true;
    }
    else
    {
        Command<LOAD_AUDIO_STREAM>(path.c_str(), &handle);
        if (handle == NULL)
        {
            LOG_VERBOSE("Failed to load sound '{}'", path);
        }
        else
        {
            is3DSupported[path] = false;
        }
    }
    return handle;
}

bool AudioMgr::ShouldPlaySound()
{
    return gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects", false);
}

void AudioMgr::PlayFileSound(const std::string &path, CEntity *pEntity, float volume, bool cached)
{
    if (!ShouldPlaySound())
    {
        return;
    }

    StreamHandle handle = NULL;

    if (cached)
    {
        if (!cache.contains(path))
        {
            StreamHandle temp = Load(path);
            if (temp == NULL)
            {
                return;
            }
            cache[path] = temp;
        }
        handle = cache[path];
    }
    else
    {
        handle = Load(path);
        if (handle != NULL)
        {
            needToFree.push_back(handle);
        }
    }

    if (handle == NULL)
    {
        return;
    }

    int state = eAudioStreamState::Stopped;
    Command<GET_AUDIO_STREAM_STATE>(handle, &state);

    if (state != eAudioStreamState::Playing)
    {
        SetVolume(handle, volume);
        if (pEntity == nullptr)
        {
            pEntity = FindPlayerPed();
        }

        // We're verifying the type so static_cast should be fine 
        if (pEntity->m_nType == ENTITY_TYPE_VEHICLE)
        {
            int hEntity = CPools::GetVehicleRef(static_cast<CVehicle *>(pEntity));
            if (hEntity != NULL)
            {
                Command<SET_PLAY_3D_AUDIO_STREAM_AT_CAR>(handle, hEntity);
            }
        }
        else if (pEntity->m_nType == ENTITY_TYPE_PED)
        {
            int hEntity = CPools::GetPedRef(static_cast<CPed *>(pEntity));
            if (hEntity != NULL)
            {
                Command<SET_PLAY_3D_AUDIO_STREAM_AT_CHAR>(handle, hEntity);
            }
        }
        else if (pEntity->m_nType == ENTITY_TYPE_OBJECT)
        {
            int hEntity = CPools::GetObjectRef(static_cast<CObject *>(pEntity));
            if (hEntity != NULL)
            {
                Command<SET_PLAY_3D_AUDIO_STREAM_AT_OBJECT>(handle, hEntity);
            }
        }
        else
        {
            const CVector &pos = pEntity->GetPosition();
            Command<SET_PLAY_3D_AUDIO_STREAM_AT_COORDS>(handle, pos.x, pos.y, pos.z);
        }
        Command<SET_AUDIO_STREAM_STATE>(handle, static_cast<int>(eAudioStreamState::Playing));
    }
}

void AudioMgr::SetVolume(StreamHandle handle, float volume)
{
    if (handle != NULL)
    {
        static float mult = gConfig.ReadFloat("TWEAKS", "SoundMult", 1.0f);
        Command<SET_AUDIO_STREAM_VOLUME>(handle, *(BYTE *)0xBA6797 / 64.0f * volume * mult);
    }
}