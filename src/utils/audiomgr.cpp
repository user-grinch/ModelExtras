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
    using tBASS_Pause = BOOL(WINAPI *)();
    using tBASS_Start = BOOL(WINAPI *)();

    static tBASS_Init fnInit = nullptr;
    static tBASS_StreamCreateFile fnStreamCreate = nullptr;
    static tBASS_ChannelPlay fnChannelPlay = nullptr;
    static tBASS_ChannelPause fnChannelPause = nullptr;
    static tBASS_ChannelSetAttribute fnChannelSetAttr = nullptr;
    static tBASS_ChannelIsActive fnChannelIsActive = nullptr;
    static tBASS_StreamFree fnStreamFree = nullptr;
    static tBASS_Pause fnPause = nullptr;
    static tBASS_Start fnStart = nullptr;

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
            fnPause = (tBASS_Pause)GetProcAddress(hBass, "BASS_Pause");
            fnStart = (tBASS_Start)GetProcAddress(hBass, "BASS_Start");

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
    gbSoundEffectsEnabled = gConfig.ReadBoolean("SOUND", "SoundEffects", false);
    gfSoundMult = gConfig.ReadFloat("SOUND", "SoundMult", 0.6f);
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

    Events::drawingEvent += []
    {
        static bool bWasPaused = false;
        bool bIsPaused = !Util::IsWindowFocused() || *(bool *)0xBA67A4 || CTimer::m_UserPause || CTimer::m_CodePause;

        if (bIsPaused != bWasPaused)
        {
            bWasPaused = bIsPaused;
            if (BassAPI::bReady)
            {
                if (bIsPaused)
                {
                    if (BassAPI::fnPause)
                    {
                        BassAPI::fnPause();
                    }
                }
                else
                {
                    if (BassAPI::fnStart)
                    {
                        BassAPI::fnStart();
                    }
                }
            }
        }
    };

    Events::processScriptsEvent += []
    {
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
    if (!gbSoundEffectsEnabled)
    {
        return false;
    }
    if (!Util::IsWindowFocused() || *(bool *)0xBA67A4 || CTimer::m_UserPause || CTimer::m_CodePause)
    {
        return false;
    }
    return true;
}

void AudioMgr::Play3DSound(const std::string &path, const CVector &worldPos, CEntity *pEntity, float baseVolume, float maxDistance)
{
    if (!ShouldPlaySound() || path.empty())
    {
        return;
    }

    float distSq = MathUtil::DistanceSquared(worldPos, TheCamera.GetPosition());
    if (distSq > (maxDistance * maxDistance))
    {
        return; // Beyond maximum audible range: cull
    }
    float dist = std::sqrt(distSq);

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
        CVector toSound = worldPos - TheCamera.GetPosition();
        CVector camRight = TheCamera.m_mCameraMatrix.right;
        float rightDot = (toSound.x * camRight.x + toSound.y * camRight.y + toSound.z * camRight.z) / dist;
        pan = std::clamp(-rightDot, -1.0f, 1.0f);
    }

    // Calibrated volume scaling with in-game SFX master volume (0xBA6797)
    constexpr float INV_64 = 1.0f / 64.0f;
    float masterSfxVol = *(BYTE *)0xBA6797 * INV_64;
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

    constexpr float INV_64 = 1.0f / 64.0f;
    float masterSfxVol = *(BYTE *)0xBA6797 * INV_64;
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

std::string AudioMgr::GetSirenAudioPath(int modeIndex)
{
    if (modeIndex < 1 || modeIndex > 9)
    {
        return "";
    }

    std::string baseName = "audio/siren" + std::to_string(modeIndex);
    const char *extensions[] = {".wav", ".mp3", ".ogg"};

    for (const char *ext : extensions)
    {
        std::string relPath = "ModelExtras/" + baseName + ext;
        std::string fullPath = PLUGIN_PATH((char *)relPath.c_str());
        DWORD attr = GetFileAttributesA(fullPath.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            return fullPath;
        }
    }
    return "";
}

StreamHandle AudioMgr::PlayLoopStream(const std::string &path, const CVector &worldPos, float baseVolume, float maxDistance)
{
    if (path.empty() || !BassAPI::bReady || !BassAPI::fnStreamCreate || !BassAPI::fnChannelPlay)
    {
        return 0;
    }

    float distSq = MathUtil::DistanceSquared(worldPos, TheCamera.GetPosition());
    if (distSq > (maxDistance * maxDistance))
    {
        return 0;
    }

    float dist = std::sqrt(distSq);
    const float nearDist = 5.0f;
    float distFactor = 1.0f;
    if (dist > nearDist)
    {
        float ratio = std::clamp((dist - nearDist) / (maxDistance - nearDist), 0.0f, 1.0f);
        distFactor = (1.0f - ratio) * (1.0f - ratio);
    }

    float pan = 0.0f;
    if (dist > 0.1f)
    {
        CVector toSound = worldPos - TheCamera.GetPosition();
        CVector camRight = TheCamera.m_mCameraMatrix.right;
        float rightDot = (toSound.x * camRight.x + toSound.y * camRight.y + toSound.z * camRight.z) / dist;
        pan = std::clamp(-rightDot, -1.0f, 1.0f);
    }

    constexpr float INV_64 = 1.0f / 64.0f;
    float masterSfxVol = *(BYTE *)0xBA6797 * INV_64;
    float finalVolume = baseVolume * distFactor * gfSoundMult * masterSfxVol;

    constexpr DWORD BASS_SAMPLE_LOOP = 4;
    BassAPI::HSTREAM stream = BassAPI::fnStreamCreate(FALSE, path.c_str(), 0, 0, BASS_SAMPLE_LOOP);
    if (!stream)
    {
        std::string altPath = path;
        std::replace(altPath.begin(), altPath.end(), '/', '\\');
        stream = BassAPI::fnStreamCreate(FALSE, altPath.c_str(), 0, 0, BASS_SAMPLE_LOOP);
    }

    if (stream)
    {
        BassAPI::fnChannelSetAttr(stream, 2 /* BASS_ATTRIB_VOL */, std::clamp(finalVolume, 0.0f, 1.0f));
        BassAPI::fnChannelSetAttr(stream, 3 /* BASS_ATTRIB_PAN */, pan);
        BassAPI::fnChannelPlay(stream, FALSE);
        needToFree.push_back(stream);
        return stream;
    }

    return 0;
}

void AudioMgr::UpdateLoopStream(StreamHandle stream, const CVector &worldPos, float baseVolume, float maxDistance)
{
    if (!stream || !BassAPI::bReady || !BassAPI::fnChannelSetAttr || !BassAPI::fnChannelIsActive)
    {
        return;
    }

    if (BassAPI::fnChannelIsActive(stream) != 1 /* BASS_ACTIVE_PLAYING */)
    {
        return;
    }

    if (!ShouldPlaySound())
    {
        BassAPI::fnChannelSetAttr(stream, 2 /* BASS_ATTRIB_VOL */, 0.0f);
        return;
    }

    float distSq = MathUtil::DistanceSquared(worldPos, TheCamera.GetPosition());
    float dist = std::sqrt(distSq);
    const float nearDist = 5.0f;
    float distFactor = 1.0f;
    if (dist > nearDist)
    {
        float ratio = std::clamp((dist - nearDist) / (maxDistance - nearDist), 0.0f, 1.0f);
        distFactor = (1.0f - ratio) * (1.0f - ratio);
    }

    float pan = 0.0f;
    if (dist > 0.1f)
    {
        CVector toSound = worldPos - TheCamera.GetPosition();
        CVector camRight = TheCamera.m_mCameraMatrix.right;
        float rightDot = (toSound.x * camRight.x + toSound.y * camRight.y + toSound.z * camRight.z) / dist;
        pan = std::clamp(-rightDot, -1.0f, 1.0f);
    }

    constexpr float INV_64 = 1.0f / 64.0f;
    float masterSfxVol = *(BYTE *)0xBA6797 * INV_64;
    float finalVolume = baseVolume * distFactor * gfSoundMult * masterSfxVol;

    BassAPI::fnChannelSetAttr(stream, 2 /* BASS_ATTRIB_VOL */, std::clamp(finalVolume, 0.0f, 1.0f));
    BassAPI::fnChannelSetAttr(stream, 3 /* BASS_ATTRIB_PAN */, pan);
}

void AudioMgr::StopLoopStream(StreamHandle &stream)
{
    if (!stream)
    {
        return;
    }

    if (BassAPI::bReady)
    {
        if (BassAPI::fnChannelPause)
        {
            BassAPI::fnChannelPause(stream);
        }
        if (BassAPI::fnStreamFree)
        {
            BassAPI::fnStreamFree(stream);
        }
    }
    auto it = std::find(needToFree.begin(), needToFree.end(), stream);
    if (it != needToFree.end())
    {
        needToFree.erase(it);
    }
    stream = 0;
}

StreamHandle AudioMgr::PlaySirenStream(const std::string &path, const CVector &worldPos, float baseVolume, float maxDistance)
{
    return PlayLoopStream(path, worldPos, baseVolume, maxDistance);
}

void AudioMgr::UpdateSirenStream(StreamHandle stream, const CVector &worldPos, float baseVolume, float maxDistance)
{
    UpdateLoopStream(stream, worldPos, baseVolume, maxDistance);
}

void AudioMgr::StopSirenStream(StreamHandle &stream)
{
    StopLoopStream(stream);
}