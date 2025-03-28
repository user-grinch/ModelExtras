#pragma once
#include "../../interface/ifeature.hpp"
#include "weaponExtender.h"
#include <vector>
#include <filesystem>
#include <map>
#include <queue>

using StreamHandle = int;

class WeaponSoundSystem
{
private:
    static inline std::deque<StreamHandle> m_NeedToFree;
    static inline std::map<eWeaponType, std::map<std::string, std::string>> m_vecRegisteredWeapons;
    static std::string *FindAudio(eWeaponType weaponType, const std::string &&audioType);
    static void PlayAudioStream(std::string *path, CPed *pPed);

public:
    static void Initialize();
    static void Register(const std::filesystem::path &filepath, std::string weaponName);
};