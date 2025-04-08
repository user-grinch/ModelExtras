#pragma once
#include <vector>
#include <queue>
#include <CEntity.h>

using StreamHandle = int;

enum eAudioStreamState
{
    Stopped = -1,
    Playing = 1,
    Paused = 2,
};

class AudioMgr
{
private:
    static inline std::deque<StreamHandle> m_NeedToFree;

public:
    static void Initialize();
    static StreamHandle Load(std::string *pPath);
    static void SetVolume(StreamHandle handle, float volume);
    static void Play(StreamHandle handle, CEntity *pEntity, float volume = 1.0f);
    static void PlayOnVehicle(StreamHandle handle, CVehicle *pVeh, float volume = 1.0f);
    static void PlayClickSound();
    static void LoadAndPlay(std::string *path, CEntity *pPed);
    static void LoadAndPlayOnVehicle(std::string *pPath, CVehicle *pVeh);
};