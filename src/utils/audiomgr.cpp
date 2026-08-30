#include "pch.h"
#include "utils/audiomgr.h"
#include "defines.h"
#include <CAudioEngine.h>
#include <CCamera.h>
#include <algorithm>

using namespace plugin;

// Minimal dynamic BASS API (CLEO 4.4.4+ / CLEO 5 engine)
namespace BassAPI
{
    using HSTREAM = uint32_t;
    using QWORD = uint64_t;

    using tBASS_Init = BOOL(WINAPI *)(int, DWORD, DWORD, HWND, const void *);
    using tBASS_StreamCreateFile = HSTREAM(WINAPI *)(BOOL, const void *, QWORD, QWORD, DWORD);
    using tBASS_ChannelPlay = BOOL(WINAPI *)(DWORD, BOOL);
    using tBASS_ChannelPause = BOOL(WINAPI *)(DWORD);
    using tBASS_ChannelSetAttribute = BOOL(WINAPI *)(DWORD, DWORD, float);
    using tBASS_ChannelIsActive = DWORD(WINAPI *)(DWORD);
    using tBASS_StreamFree = BOOL(WINAPI *)(HSTREAM);

    static tBASS_Init fnInit = nullptr;
    static tBASS_StreamCreateFile fnStreamCreate = nullptr;
    static tBASS_ChannelPlay fnChannelPlay = nullptr;
    static tBASS_ChannelPause fnChannelPause = nullptr;
    static tBASS_ChannelSetAttribute fnChannelSetAttr = nullptr;
    static tBASS_ChannelIsActive fnChannelIsActive = nullptr;
    static tBASS_StreamFree fnStreamFree = nullptr;

    static bool bReady = false;

    static void Init()
    {
        if (bReady)
        {
            return;
        }

        HMODULE hBass = GetModuleHandleA("bass.dll");
        if (!hBass)
        {
            hBass = LoadLibraryA("bass.dll");
        }

        if (hBass)
        {
            fnInit = (tBASS_Init)GetProcAddress(hBass, "BASS_Init");
            fnStreamCreate = (tBASS_StreamCreateFile)GetProcAddress(hBass, "BASS_StreamCreateFile");
            fnChannelPlay = (tBASS_ChannelPlay)GetProcAddress(hBass, "BASS_ChannelPlay");
            fnChannelPause = (tBASS_ChannelPause)GetProcAddress(hBass, "BASS_ChannelPause");
            fnChannelSetAttr = (tBASS_ChannelSetAttribute)GetProcAddress(hBass, "BASS_ChannelSetAttribute");
            fnChannelIsActive = (tBASS_ChannelIsActive)GetProcAddress(hBass, "BASS_ChannelIsActive");
            fnStreamFree = (tBASS_StreamFree)GetProcAddress(hBass, "BASS_StreamFree");

            if (fnInit && fnStreamCreate && fnChannelPlay && fnChannelPause && fnChannelSetAttr && fnChannelIsActive && fnStreamFree)
            {
                HWND hWnd = (RsGlobal.ps && RsGlobal.ps->window) ? RsGlobal.ps->window : NULL;
                fnInit(-1, 44100, 0, hWnd, nullptr);
                bReady = true;
                LOG_VERBOSE("AudioMgr: BASS audio engine initialized successfully.");
            }
        }
    }
}

static bool gbSoundEffectsEnabled = false;
static float gfSoundMult = 1.0f;

void AudioMgr::ReloadConfig()
{
    gbSoundEffectsEnabled = gConfig.ReadBoolean("SOUND", "SoundEffects", gConfig.ReadBoolean("FEATURES", "SoundEffects", false));
    gfSoundMult = gConfig.ReadFloat("SOUND", "SoundMult", gConfig.ReadFloat("TWEAKS", "SoundMult", 0.6f));
}

void AudioMgr::Init()
{
    Events::initGameEvent += []
    {
        BassAPI::Init();
        ReloadConfig();
    };

    Events::reInitGameEvent += []
    {
        for (auto stream : needToFree)
        {
            if (stream && BassAPI::fnStreamFree)
            {
                BassAPI::fnStreamFree(stream);
            }
        }
        needToFree.clear();
    };

    Events::processScriptsEvent += []
    {
        static bool bWasPaused = false;
        bool bIsPaused = CTimer::m_UserPause || CTimer::m_CodePause;

        if (bIsPaused != bWasPaused)
        {
            bWasPaused = bIsPaused;
            if (BassAPI::bReady && BassAPI::fnChannelPause && BassAPI::fnChannelPlay && BassAPI::fnChannelIsActive)
            {
                for (auto stream : needToFree)
                {
                    if (stream)
                    {
                        if (bIsPaused)
                        {
                            if (BassAPI::fnChannelIsActive(stream) == 1 /* BASS_ACTIVE_PLAYING */)
                            {
                                BassAPI::fnChannelPause(stream);
                            }
                        }
                        else
                        {
                            if (BassAPI::fnChannelIsActive(stream) == 3 /* BASS_ACTIVE_PAUSED */)
                            {
                                BassAPI::fnChannelPlay(stream, FALSE);
                            }
                        }
                    }
                }
            }
        }

        static size_t prev = 0;
        size_t cur = CTimer::m_snTimeInMilliseconds;

        if (cur - prev > 250)
        {
            for (auto it = needToFree.begin(); it != needToFree.end();)
            {
                if (!*it || !BassAPI::fnChannelIsActive || BassAPI::fnChannelIsActive(*it) == 0 /* BASS_ACTIVE_STOPPED */)
                {
                    if (*it && BassAPI::fnStreamFree)
                    {
                        BassAPI::fnStreamFree(*it);
                    }
                    it = needToFree.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            prev = cur;
        }
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
    static std::string path = MOD_DATA_PATH("audio/switch_toggle.wav");
    PlayFileSound(path, 1.0f);
}

bool AudioMgr::ShouldPlaySound()
{
    return gbSoundEffectsEnabled;
}

void AudioMgr::Play3DSound(const std::string &path, const CVector &worldPos, CEntity *pEntity, float baseVolume, float maxDistance)
{
    if (!ShouldPlaySound() || path.empty())
    {
        return;
    }

    CVector listenerPos = TheCamera.GetPosition();
    float dist = CVector::Distance(worldPos, listenerPos);
    if (dist > maxDistance)
    {
        return; // Beyond maximum audible range: cull
    }

    // Natural smooth distance attenuation
    // Full volume within near radius (5m), then linear acoustic decay up to maxDistance
    const float nearDist = 5.0f;
    float distFactor = 1.0f;
    if (dist > nearDist)
    {
        float ratio = std::clamp((dist - nearDist) / (maxDistance - nearDist), 0.0f, 1.0f);
        distFactor = 1.0f - ratio;
    }

    // 3D camera-relative stereo panning (-1.0 = left, 0.0 = center, +1.0 = right)
    float pan = 0.0f;
    if (dist > 0.1f)
    {
        CVector toSound = worldPos - listenerPos;
        CVector camRight = TheCamera.m_mCameraMatrix.right;
        float rightDot = (toSound.x * camRight.x + toSound.y * camRight.y + toSound.z * camRight.z) / dist;
        pan = std::clamp(rightDot, -1.0f, 1.0f);
    }

    // Calibrated volume scaling with in-game SFX master volume (0xBA6797)
    float masterSfxVol = *(BYTE *)0xBA6797 / 64.0f;
    float finalVolume = baseVolume * distFactor * gfSoundMult * masterSfxVol;
    if (finalVolume < 0.005f)
    {
        return;
    }

    if (BassAPI::bReady && BassAPI::fnStreamCreate)
    {
        BassAPI::HSTREAM stream = BassAPI::fnStreamCreate(FALSE, path.c_str(), 0, 0, 0);
        if (!stream)
        {
            std::string altPath = path;
            std::replace(altPath.begin(), altPath.end(), '/', '\\');
            stream = BassAPI::fnStreamCreate(FALSE, altPath.c_str(), 0, 0, 0);
        }

        if (stream)
        {
            BassAPI::fnChannelSetAttr(stream, 2 /* BASS_ATTRIB_VOL */, std::clamp(finalVolume, 0.0f, 1.0f));
            BassAPI::fnChannelSetAttr(stream, 3 /* BASS_ATTRIB_PAN */, pan);
            BassAPI::fnChannelPlay(stream, TRUE);
            needToFree.push_back(stream);
        }
    }
}

void AudioMgr::PlayFileSound(const std::string &path, float volume)
{
    if (!ShouldPlaySound() || path.empty())
    {
        return;
    }

    float masterSfxVol = *(BYTE *)0xBA6797 / 64.0f;
    float finalVolume = volume * gfSoundMult * masterSfxVol;
    if (finalVolume < 0.005f)
    {
        return;
    }

    if (BassAPI::bReady && BassAPI::fnStreamCreate)
    {
        BassAPI::HSTREAM stream = BassAPI::fnStreamCreate(FALSE, path.c_str(), 0, 0, 0);
        if (!stream)
        {
            std::string altPath = path;
            std::replace(altPath.begin(), altPath.end(), '/', '\\');
            stream = BassAPI::fnStreamCreate(FALSE, altPath.c_str(), 0, 0, 0);
        }

        if (stream)
        {
            BassAPI::fnChannelSetAttr(stream, 2 /* BASS_ATTRIB_VOL */, std::clamp(finalVolume, 0.0f, 1.0f));
            BassAPI::fnChannelSetAttr(stream, 3 /* BASS_ATTRIB_PAN */, 0.0f);
            BassAPI::fnChannelPlay(stream, TRUE);
            needToFree.push_back(stream);
        }
    }
}