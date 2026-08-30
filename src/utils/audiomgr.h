#pragma once
#include <deque>
#include <string>
#include <CEntity.h>
#include <CVector.h>

using StreamHandle = uint32_t;

class AudioMgr
{
private:
    static inline std::deque<StreamHandle> needToFree;

    static bool ShouldPlaySound();
    
public:
    static void Init();
    static void ReloadConfig();
    static void Play3DSound(const std::string &path, const CVector &worldPos, CEntity *pEntity = nullptr, float baseVolume = 1.0f, float maxDistance = 40.0f);
    static void PlayFileSound(const std::string &path, float volume = 1.0f);
    static void PlayClickSound();
    static void PlaySwitchSound(CEntity *pEntity = nullptr);
};