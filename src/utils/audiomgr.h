#pragma once
#include <vector>
#include <deque>
#include <string>
#include <unordered_map>
#include <CEntity.h>
#include <CVector.h>

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
    static inline std::deque<StreamHandle> needToFree;

    static void SetVolume(StreamHandle handle, float volume);
    static bool ShouldPlaySound();
    
public:
    static void Init();
    static void ReloadConfig();
    static void Play3DSound(const std::string &path, const CVector &worldPos, CEntity *pEntity = nullptr, float baseVolume = 1.0f, float maxDistance = 40.0f);
    static void PlayFileSound(const std::string &path, CEntity *pEntity = nullptr, float volume = 1.0f, bool cached = false);
    static void PlayClickSound();
    static void PlaySwitchSound(CEntity *pEntity = nullptr);
};