#include "pch.h"
#include "sound.h"

#include <plugin.h>
#include <CModelInfo.h>
#include <CAEAudioHardware.h>
#include <CAEPedAudioEntity.h>

inline float GetGameVolume() {
    return *(BYTE*)0xBA6797 / 64.0f;
}

C3DAudioStream* WeaponSoundSystem::FindAudio(eWeaponType weaponType, std::string audioType) {
    auto e = m_vecRegisteredWeapons.find(weaponType);
    if (e != m_vecRegisteredWeapons.end()) {
        for (auto &f: e->second) {
            if (f.first == audioType) {
                return f.second;
            }
        }
    }
    return nullptr;
}

size_t WeaponSoundSystem::GetCount() {
    return m_vecRegisteredWeapons.size();
}

void WeaponSoundSystem::Register(const std::filesystem::path& filepath) {
    int model = NULL;
    std::string audioType = filepath.stem().string();
    std::string weaponName = filepath.parent_path().filename().string();
    
    if (weaponName.empty()) {
        gLogger->warn("Failed to register weapon sound '{}' for {}", audioType, weaponName);
        return;
    }

    CModelInfo::GetModelInfo(weaponName.data(), &model);
    eWeaponType weapontype = CWeaponInfo::FindWeaponType((char*)weaponName.c_str());
    if (model > NULL && weapontype > eWeaponType::WEAPON_UNARMED) {
        m_vecRegisteredWeapons[weapontype][audioType] = 
            (C3DAudioStream*)SoundSystem.CreateStream(filepath.string().c_str(), true);
        LOG_VERBOSE("Registered weapon sound '{}' for {}", audioType, weaponName);
    }
}

void WeaponSoundSystem::Initialize() {
    plugin::Events::initGameEvent += [] {
        if (std::filesystem::exists(MOD_DATA_PATH("audio/weapon/"))) {
            for (auto e : std::filesystem::recursive_directory_iterator(MOD_DATA_PATH("audio/weapon/"))) {
                std::string ext = e.path().extension().string();
                if (ext == ".wav" || ext == ".ogg") {
                    WeaponSoundSystem::Register(e.path());
                }
            }
        }
        
        if (std::filesystem::exists(GAME_PATH((char*)"EarShot/"))) {
            for (auto e : std::filesystem::recursive_directory_iterator(GAME_PATH((char*)"EarShot/"))) {
                std::string ext = e.path().extension().string();
                if (ext == ".wav" || ext == ".ogg") {
                    WeaponSoundSystem::Register(e.path());
                }
            }
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4DFAC6, plugin::H_CALL, 0x4E6A3D, plugin::H_CALL>, plugin::PRIORITY_BEFORE, 
        plugin::ArgPick4N<CAEWeaponAudioEntity*, 0, eWeaponType, 1, CPhysical*, 2, int, 3>, 
        void(CAEWeaponAudioEntity*, eWeaponType, CPhysical*, int)> CAEWeaponAudioEntity_WeaponFireEvent;

    CAEWeaponAudioEntity_WeaponFireEvent += [](CAEWeaponAudioEntity* pAudioEnt, eWeaponType weaponType, CPhysical* entity, int) {
        if (pAudioEnt && pAudioEnt->m_pPed) {
            C3DAudioStream *pStream = WeaponSoundSystem::FindAudio(weaponType, "shoot");
            if (pStream) {
                pStream->SetProgress(0.0f);
                pStream->Set3dPosition(pAudioEnt->m_pPed->GetPosition());
                pStream->SetVolume(GetGameVolume());
                pStream->Play();
            }
            plugin::patch::SetRaw(0x504F80, pStream ? (char*)"\xC2\x0C\x00" : (char*)"\x8B\x44\x24", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4DFAD6, plugin::H_CALL, 0x4E6A5B, plugin::H_CALL>, plugin::PRIORITY_BEFORE, 
        plugin::ArgPick4N<CAEWeaponAudioEntity*, 0, eWeaponType, 1, CPed*, 2, int, 3>, 
        void(CAEWeaponAudioEntity*, eWeaponType, CPed*, int)> CAEWeaponAudioEntity_WeaponFireReload;

    CAEWeaponAudioEntity_WeaponFireReload += [](CAEWeaponAudioEntity* pAudioEnt, eWeaponType weaponType, CPed* pPed, int unk) {
        if (pAudioEnt && pAudioEnt->m_pPed) {
            C3DAudioStream *pStream = WeaponSoundSystem::FindAudio(weaponType, "reload");
            if (pStream) {
                pStream->SetProgress(0.0f);
                pStream->SetType(eStreamType::SoundEffect);
                pStream->Set3dPosition(pAudioEnt->m_pPed->GetPosition());
                pStream->SetVolume(GetGameVolume());
                pStream->Play();
            } 
            plugin::patch::SetRaw(0x503690, pStream ? (char*)"\xC2\x0C\x00" : (char*)"\x51\x53\x55", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4DFAE6, plugin::H_CALL>, plugin::PRIORITY_BEFORE, 
        plugin::ArgPick4N<CAEWeaponAudioEntity*, 0, eWeaponType, 1, CPed*, 2, int, 3>, 
        void(CAEWeaponAudioEntity*, eWeaponType, CPed*, int)> CAEWeaponAudioEntity_WeaponProjectile;

    CAEWeaponAudioEntity_WeaponProjectile += [](CAEWeaponAudioEntity* pAudioEnt, eWeaponType weaponType, CPed* pPed, int unk) {
        if (pAudioEnt && pAudioEnt->m_pPed) {
            C3DAudioStream *pStream = WeaponSoundSystem::FindAudio(weaponType, "projectile");
            if (pStream) {
                pStream->SetProgress(0.0f);
                pStream->SetType(eStreamType::SoundEffect);
                pStream->Set3dPosition(pAudioEnt->m_pPed->GetPosition());
                pStream->SetVolume(GetGameVolume());
                pStream->Play();
            } 
            plugin::patch::SetRaw(0x4DF060, pStream ? (char*)"\xC2\x0C\x00" : (char*)"\x8B\x44\x24", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4E2CA9, plugin::H_CALL>, plugin::PRIORITY_BEFORE, 
        plugin::ArgPick6N<CAEPedAudioEntity*, 0, int, 1, CPhysical*, 2, char, 3, float, 4, unsigned int, 5>, 
        void(CAEPedAudioEntity*, int, CPhysical*, char, float, unsigned int)> CAEPedAudioEntity_HitEvent;

    CAEPedAudioEntity_HitEvent += [](CAEPedAudioEntity* pAudioEnt, int, CPhysical* entity, char a, float b, unsigned int c) {
        if (pAudioEnt && pAudioEnt->m_pPed) {
            eWeaponType weaponType = pAudioEnt->m_pPed->m_aWeapons[pAudioEnt->m_pPed->m_nActiveWeaponSlot].m_eWeaponType;
            C3DAudioStream *pStream = WeaponSoundSystem::FindAudio(weaponType, "hit");
            if (pStream) {
                pStream->SetProgress(0.0f);
                pStream->SetType(eStreamType::SoundEffect);
                pStream->Set3dPosition(pAudioEnt->m_pPed->GetPosition());
                pStream->SetVolume(GetGameVolume());
                pStream->Play();
            }
            plugin::patch::SetRaw(0x4E1CC0, pStream ? (char*)"\xC2\x14\x00" : (char*)"\x83\xEC\x14", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4E2C7C, plugin::H_CALL>, plugin::PRIORITY_BEFORE, 
        plugin::ArgPick4N<CAEPedAudioEntity*, 0, int, 1, int, 2, int, 3>, char(CAEPedAudioEntity*, int, int, int)> CAEPedAudioEntity_SwingEvent;

    CAEPedAudioEntity_SwingEvent += [](CAEPedAudioEntity* pAudioEnt, int a, int b, int c) {
        if (pAudioEnt && pAudioEnt->m_pPed) {
            eWeaponType weaponType = pAudioEnt->m_pPed->m_aWeapons[pAudioEnt->m_pPed->m_nActiveWeaponSlot].m_eWeaponType;
            C3DAudioStream *pStream = WeaponSoundSystem::FindAudio(weaponType, "swing");
            if (pStream) {
                pStream->SetProgress(0.0f);
                pStream->SetType(eStreamType::SoundEffect);
                pStream->Set3dPosition(pAudioEnt->m_pPed->GetPosition());
                pStream->SetVolume(GetGameVolume());
                pStream->Play();
            }
            plugin::patch::SetRaw(0x4E1A40, pStream ? (char*)"\xC2\x0C\x00" : (char*)"\x83\xEC\x08", 3);
        }
    };
}