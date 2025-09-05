#pragma once
#include <CEntity.h>
#include <queue>
#include <vector>

using StreamHandle = int;

enum eAudioStreamState {
  Stopped = -1,
  Playing = 1,
  Paused = 2,
};

class AudioMgr {
private:
  static inline std::deque<StreamHandle> m_NeedToFree;
  static inline std::unordered_map<std::string, StreamHandle> m_Cache;

  static StreamHandle Load(const std::string &path);
  static void SetVolume(StreamHandle handle, float volume);

public:
  static void Initialize();
  static void PlayFileSound(const std::string &path, CEntity *pEntity,
                            float volume = 1.0f, bool cached = false);
  static void PlayClickSound();
  static void PlaySwitchSound(CEntity *pEntity = NULL);
};