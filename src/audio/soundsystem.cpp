#include "pch.h"
#include <CAEAudioHardware.h>
#include <CCamera.h>
#include "soundsystem.h"
#include "audiostream.h"
#include "audiostream3d.h"

CSoundSystem SoundSystem;
bool CSoundSystem::useFloatAudio = false;
bool CSoundSystem::allowNetworkSources = true;
BASS_3DVECTOR CSoundSystem::pos(0.0, 0.0, 0.0);
BASS_3DVECTOR CSoundSystem::vel(0.0, 0.0, 0.0);
BASS_3DVECTOR CSoundSystem::front(0.0, -1.0, 0.0);
BASS_3DVECTOR CSoundSystem::top(0.0, 0.0, 1.0);
eStreamType CSoundSystem::defaultStreamType = eStreamType::SoundEffect;
float CSoundSystem::masterSpeed = 1.0f;
float CSoundSystem::masterVolumeSfx = 1.0f;
float CSoundSystem::masterVolumeMusic = 1.0f;

void EnumerateBassDevices(int& total, int& enabled, int& default_device)
{
    BASS_DEVICEINFO info;
    LOG_NO_LEVEL("");
    for (default_device = -1, enabled = 0, total = 0; BASS_GetDeviceInfo(total, &info); ++total)
    {
        if (info.flags & BASS_DEVICE_ENABLED) ++enabled;
        if (info.flags & BASS_DEVICE_DEFAULT) default_device = total;
        gLogger->info("Found sound device {}{}: {}", total, default_device == total ?
            " (default)" : "", info.name);
    }
}

bool isNetworkSource(const char* path)
{
    return _strnicmp("http:", path, 5) == 0 ||
        _strnicmp("https:", path, 6) == 0;
}

CSoundSystem::~CSoundSystem()
{
    Clear();

    if (initialized)
    {
        BASS_Free();
        initialized = false;
    }
}

bool CSoundSystem::Init()
{
    if (initialized) return true; // already done

    defaultStreamType = eStreamType::None;
    allowNetworkSources = true;

    int default_device, total_devices, enabled_devices;
    EnumerateBassDevices(total_devices, enabled_devices, default_device);

    int forceDevice = -1;
    BASS_DEVICEINFO info = { nullptr, nullptr, 0 };
    if (forceDevice != -1 && BASS_GetDeviceInfo(forceDevice, &info) && (info.flags & BASS_DEVICE_ENABLED))
        default_device = forceDevice;

    gLogger->info("On system found {} devices, {} enabled devices, assuming device to use: {} ({})",
        total_devices, enabled_devices, default_device, BASS_GetDeviceInfo(default_device, &info) ?
        info.name : "Unknown device");

    if (BASS_Init(default_device, 44100, BASS_DEVICE_3D, RsGlobal.ps->window, nullptr) &&
        BASS_Set3DFactors(1.0f, 3.0f, 80.0f) &&
        BASS_Set3DPosition(&pos, &vel, &front, &top))
    {
        gLogger->info("SoundSystem initialized");

        // Can we use floating-point (HQ) audio streams?
        DWORD floatable = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, NULL, NULL); // floating-point channel support? 0 = no, else yes
        if (floatable)
        {
            gLogger->info("Floating-point audio supported!");
            useFloatAudio = true;
            BASS_StreamFree(floatable);
        }
        else gLogger->info("Floating-point audio not supported!");

        if (BASS_GetInfo(&SoundDevice))
        {
            if (SoundDevice.flags & DSCAPS_EMULDRIVER)
                gLogger->info("Audio drivers not installed - using DirectSound emulation");
            if (!SoundDevice.eax)
                gLogger->info("Audio hardware acceleration disabled (no EAX)");
        }

        initialized = true;
        BASS_Apply3D();
        LOG_NO_LEVEL("");
        return true;
    }

    int code = BASS_ErrorGetCode();
    if (code == BASS_ERROR_ALREADY) {
        gLogger->info("BASS already initialized. Skipping init process.");
    }
    else {
        gLogger->warn("Could not initialize BASS sound system. Error code: {}", BASS_ErrorGetCode());
    }
    LOG_NO_LEVEL("");
    return false;
}

bool CSoundSystem::Initialized()
{
    return initialized;
}

CAudioStream* CSoundSystem::CreateStream(const char* filename, bool in3d)
{
    CAudioStream* result = in3d ? new C3DAudioStream(filename) : new CAudioStream(filename);
    if (!result->IsOk())
    {
        delete result;
        return nullptr;
    }

    streams.insert(result);
    return result;
}

void CSoundSystem::DestroyStream(CAudioStream* stream)
{
    if (streams.erase(stream))
        delete stream;
    else
        gLogger->info("Unloading of stream that is not in list of loaded streams");
}

bool CSoundSystem::HasStream(CAudioStream* stream)
{
    return streams.find(stream) != streams.end();
}

void CSoundSystem::Clear()
{
    for (auto stream : streams)
    {
        delete stream;
    };
    streams.clear();
}

void CSoundSystem::Resume()
{
    paused = false;
    for (auto stream : streams)
    {
        if (stream->GetState() == CAudioStream::Playing) stream->Resume();
    }
}

void CSoundSystem::Pause()
{
    paused = true;
    for (auto stream : streams)
    {
        stream->Pause(false);
    };
}

void CSoundSystem::Process()
{
    if (CTimer::m_UserPause || CTimer::m_CodePause) // covers menu pausing, no disc in drive pausing, etc.
    {
        if (!paused) Pause();
    }
    else // not in menu
    {
        if (paused) Resume();

        // get game globals
        masterSpeed = CTimer::ms_fTimeScale;
        masterVolumeSfx = AEAudioHardware.m_fEffectMasterScalingFactor * 0.5f; // fit to game's sfx volume
        masterVolumeMusic = AEAudioHardware.m_fMusicMasterScalingFactor * 0.5f;

        // camera movements
        CMatrixLink* pMatrix = nullptr;
        CVector* pVec = nullptr;
        if (TheCamera.m_matrix)
        {
            pMatrix = TheCamera.m_matrix;
            pVec = &pMatrix->pos;
        }
        else pVec = &TheCamera.m_placement.m_vPosn;

        BASS_3DVECTOR prevPos = pos;
        pos = BASS_3DVECTOR(pVec->y, pVec->z, pVec->x);

        // calculate velocity
        vel = prevPos;
        vel.x -= pos.x;
        vel.y -= pos.y;
        vel.z -= pos.z;
        auto timeDelta = 0.001f * (CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped);
        vel.x *= timeDelta;
        vel.y *= timeDelta;
        vel.z *= timeDelta;

        // setup the ears
        if (!TheCamera.m_bJust_Switched && !TheCamera.m_bCameraJustRestored) // avoid camera change/jump cut velocity glitches
        {
            BASS_3DVECTOR front = BASS_3DVECTOR(pMatrix->at.y, pMatrix->at.z, pMatrix->at.x);
            BASS_3DVECTOR top = BASS_3DVECTOR(pMatrix->up.y, pMatrix->up.z, pMatrix->up.x);
            BASS_Set3DPosition(&pos, &vel, pMatrix ? &front : nullptr, pMatrix ? &top : nullptr);
        }

        // process streams
        for (auto stream : streams) stream->Process();

        // apply above changes
        BASS_Apply3D();
    }
}