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
    static inline std::deque<StreamHandle> needToFree;
    static inline std::unordered_map<std::string, StreamHandle> cache;

    static StreamHandle Load(const std::string &path);
    static void SetVolume(StreamHandle handle, float volume);
    static bool ShouldPlaySound();
    
public:
    static void Initialize();
    static void PlayFileSound(const std::string &path, CEntity *pEntity, float volume = 1.0f, bool cached = false);
    static void PlayClickSound();
    static void PlaySwitchSound(CEntity *pEntity = NULL);
};