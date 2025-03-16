#pragma once
#include "../../interface/ifeature.hpp"
#include "weaponExtender.h"
#include <vector>
#include <filesystem>
#include <map>

using StreamHandle = int;

class WeaponSoundSystem
{
private:
    static inline std::map<eWeaponType, std::map<std::string, StreamHandle>> m_vecRegisteredWeapons;
    static StreamHandle FindAudio(eWeaponType weaponType, std::string audioType);
    static void PlayAudioStream(StreamHandle handle, CPed *pPed);

public:
    static void Initialize();
    static void Register(const std::filesystem::path &filepath);
};