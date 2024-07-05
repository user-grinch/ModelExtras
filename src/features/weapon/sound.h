#pragma once
#include "../../interface/ifeature.hpp"
#include "../../weaponExtender.h"
#include <vector>
#include <filesystem>
#include <map>

#include "audiostream3d.h"
#include "soundsystem.h"

class WeaponSoundSystem {
private:
    static inline CSoundSystem m_SoundSystem;
    static inline std::map<eWeaponType, std::map<std::string, C3DAudioStream*>> m_vecRegisteredWeapons;

public:
    static void Initialize();
    static C3DAudioStream* FindAudio(eWeaponType weaponType, std::string audioType);
    static size_t GetCount();
    static void Register(const std::filesystem::path& filepath);
};