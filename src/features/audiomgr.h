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
    static void Play(StreamHandle handle, CEntity *pEntity);
    static void PlayClickSound();
    static void LoadAndPlay(std::string *path, CEntity *pPed);
};