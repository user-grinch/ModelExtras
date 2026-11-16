#include "pch.h"
#include "utils/audiomgr.h"
#include "defines.h"
#include "utils/util.h"
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

    // BASS constants (bass.dll is resolved dynamically, so no SDK headers)
    constexpr DWORD BASS_SAMPLE_MONO    = 0x2; // downmix stereo sources so PAN can steer them
    constexpr DWORD BASS_SAMPLE_LOOP    = 0x4;
    constexpr DWORD BASS_ATTRIB_VOL     = 0x2;
    constexpr DWORD BASS_ATTRIB_PAN     = 0x3;
    constexpr DWORD BASS_ACTIVE_STOPPED = 0x0;
    constexpr DWORD BASS_ACTIVE_PLAYING = 0x1;
    constexpr DWORD BASS_ACTIVE_PAUSED  = 0x3;

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
                LOG_VERBOSE("[Audio] BASS audio engine initialized successfully.");
            }
        }
    }
}

static bool gbSoundEffectsEnabled = false;
static float gfSoundMult = 1.0f;

// Full volume within nearDist, then fading to zero at maxDistance.
// rolloff 1.0 = linear fade for one-shots, 2.0 = quadratic fade so constant
// loop sounds (brake squeal) drop off faster with distance.
static float ComputeDistanceFactor(const CVector &worldPos, float maxDistance, float rolloff)
{
    float distSq = MathUtil::DistanceSquared(worldPos, TheCamera.GetPosition());
    if (distSq > maxDistance * maxDistance)
    {
        return 0.0f; // Beyond maximum audible range
    }

    float dist = std::sqrt(distSq);
    constexpr float nearDist = 5.0f;
    if (dist <= nearDist)
    {
        return 1.0f;
    }

    float ratio = std::clamp((dist - nearDist) / (maxDistance - nearDist), 0.0f, 1.0f);
    return std::pow(1.0f - ratio, rolloff);
}

// Camera-relative stereo panning (-1.0 = left, 0.0 = center, +1.0 = right).
// Follows the game's own right-vector convention (CCamera::Find3rdPersonCam-
// TargetVector): cross(front, up) of the active cam. m_mCameraMatrix rows use
// GTA's own axis layout (the look direction is stored where a plain RwMatrix
// keeps "up"), so reading its "right" row directly panned sounds to the wrong
// side.
static float ComputePan(const CVector &worldPos)
{
    CVector toSound = worldPos - TheCamera.GetPosition();
    float dist = toSound.Magnitude();
    if (dist <= 0.1f)
    {
        return 0.0f;
    }

    const CCam &cam = TheCamera.m_aCams[TheCamera.m_nActiveCam];
    CVector camRight = CVector::Cross(cam.m_vecFront, cam.m_vecUp);
    float rightLen = camRight.Magnitude();
    if (rightLen < 0.01f)
    {
        return 0.0f; // degenerate front/up pair
    }
    camRight.x /= rightLen;
    camRight.y /= rightLen;
    camRight.z /= rightLen;

    float rightDot = (toSound.x * camRight.x + toSound.y * camRight.y + toSound.z * camRight.z) / dist;
    return std::clamp(rightDot, -1.0f, 1.0f);
}

// Streams are always created mono: BASS PAN cannot reposition stereo sources,
// it only attenuates one channel, so a stereo file (door chime, brake squeal)
// keeps sounding from the side it was recorded on.
static BassAPI::HSTREAM CreateStream(const std::string &path, DWORD flags)
{
    DWORD createFlags = flags | BassAPI::BASS_SAMPLE_MONO;
    BassAPI::HSTREAM stream = BassAPI::fnStreamCreate(FALSE, path.c_str(), 0, 0, createFlags);
    if (!stream)
    {
        // Some BASS builds reject forward slashes; retry with backslashes
        std::string altPath = path;
        std::replace(altPath.begin(), altPath.end(), '/', '\\');
        stream = BassAPI::fnStreamCreate(FALSE, altPath.c_str(), 0, 0, createFlags);
    }
    return stream;
}

// In-game SFX master volume (0xBA6797)
static float GetMasterSfxVolume()
{
    constexpr float INV_64 = 1.0f / 64.0f;
    return *(BYTE *)0xBA6797 * INV_64;
}

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

    // gameProcessEvent (Idle -> CGame::Process) keeps firing while the game is
    // paused, unlike processScriptsEvent which is skipped entirely. Running here
    // lets loop streams be suspended for the pause menu and finished one-shots
    // be freed even when ESC/menu pauses the game.
    Events::gameProcessEvent += []
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
                    if (!stream)
                    {
                        continue;
                    }

                    if (bIsPaused)
                    {
                        if (BassAPI::fnChannelIsActive(stream) == BassAPI::BASS_ACTIVE_PLAYING)
                        {
                            BassAPI::fnChannelPause(stream);
                        }
                    }
                    else if (BassAPI::fnChannelIsActive(stream) == BassAPI::BASS_ACTIVE_PAUSED)
                    {
                        BassAPI::fnChannelPlay(stream, FALSE);
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
                if (!*it || !BassAPI::fnChannelIsActive || BassAPI::fnChannelIsActive(*it) == BassAPI::BASS_ACTIVE_STOPPED)
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

void AudioMgr::PlaySwitchSound()
{
    static std::string path = MOD_DATA_PATH("audio/switch_toggle.wav");
    PlayFileSound(path, 1.0f);
}

bool AudioMgr::ShouldPlaySound()
{
    if (!gbSoundEffectsEnabled || !Util::IsWindowFocused() || CTimer::m_UserPause || CTimer::m_CodePause)
    {
        return false;
    }
    return true;
}

void AudioMgr::Play3DSound(const std::string &path, const CVector &worldPos, float baseVolume, float maxDistance)
{
    if (!ShouldPlaySound() || path.empty())
    {
        return;
    }

    float distFactor = ComputeDistanceFactor(worldPos, maxDistance, 1.0f);
    if (distFactor <= 0.0f)
    {
        return; // Beyond maximum audible range: cull
    }

    float finalVolume = baseVolume * distFactor * gfSoundMult * GetMasterSfxVolume();
    if (finalVolume < 0.005f)
    {
        return;
    }

    if (BassAPI::bReady && BassAPI::fnStreamCreate)
    {
        BassAPI::HSTREAM stream = CreateStream(path, 0);
        if (stream)
        {
            BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_VOL, std::clamp(finalVolume, 0.0f, 1.0f));
            BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_PAN, ComputePan(worldPos));
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

    float finalVolume = volume * gfSoundMult * GetMasterSfxVolume();
    if (finalVolume < 0.005f)
    {
        return;
    }

    if (BassAPI::bReady && BassAPI::fnStreamCreate)
    {
        BassAPI::HSTREAM stream = CreateStream(path, 0);
        if (stream)
        {
            BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_VOL, std::clamp(finalVolume, 0.0f, 1.0f));
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

    if (!ShouldPlaySound())
    {
        return 0;
    }

    float distFactor = ComputeDistanceFactor(worldPos, maxDistance, 2.0f);
    if (distFactor <= 0.0f)
    {
        return 0;
    }

    float finalVolume = baseVolume * distFactor * gfSoundMult * GetMasterSfxVolume();

    BassAPI::HSTREAM stream = CreateStream(path, BassAPI::BASS_SAMPLE_LOOP);
    if (stream)
    {
        BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_VOL, std::clamp(finalVolume, 0.0f, 1.0f));
        BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_PAN, ComputePan(worldPos));
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

    if (BassAPI::fnChannelIsActive(stream) != BassAPI::BASS_ACTIVE_PLAYING)
    {
        return;
    }

    if (!ShouldPlaySound())
    {
        BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_VOL, 0.0f);
        return;
    }

    float finalVolume = baseVolume * ComputeDistanceFactor(worldPos, maxDistance, 2.0f) * gfSoundMult * GetMasterSfxVolume();

    BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_VOL, std::clamp(finalVolume, 0.0f, 1.0f));
    BassAPI::fnChannelSetAttr(stream, BassAPI::BASS_ATTRIB_PAN, ComputePan(worldPos));
}

void AudioMgr::StopLoopStream(StreamHandle &stream)
{
    if (!stream)
    {
        return;
    }

    if (BassAPI::bReady && BassAPI::fnStreamFree)
    {
        BassAPI::fnStreamFree(stream);
    }

    std::erase(needToFree, stream);
    stream = 0;
}
