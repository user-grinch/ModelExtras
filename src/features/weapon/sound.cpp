#include "pch.h"
#include "defines.h"
#include "sound.h"
#include <plugin.h>
#include <CAEAudioHardware.h>
#include <CAEPedAudioEntity.h>
#include "audiomgr.h"

std::string *WeaponSoundSystem::FindAudio(eWeaponType weaponType, const std::string &&audioType)
{
    auto e = m_vecRegisteredWeapons.find(weaponType);
    if (e != m_vecRegisteredWeapons.end())
    {
        for (auto &f : e->second)
        {
            if (f.first == audioType)
            {
                return &f.second;
            }
        }
    }
    return NULL;
}

void WeaponSoundSystem::Register(const std::filesystem::path &filepath, std::string weaponName)
{
    std::string audioType = filepath.stem().string();

    if (weaponName.empty())
    {
        gLogger->warn("Failed to register weapon sound '{}' for {}", audioType, weaponName);
        return;
    }
    eWeaponType weapontype = CWeaponInfo::FindWeaponType((char *)weaponName.c_str());
    if (weapontype > eWeaponType::WEAPONTYPE_UNARMED)
    {
        m_vecRegisteredWeapons[weapontype][audioType] = std::move(filepath.string());
        LOG_VERBOSE("Registering '{}' for {}", audioType, weaponName);
    }
}

void SearchWeaponSounds(const std::string &folderPath)
{
    if (std::filesystem::exists(folderPath))
    {
        for (const auto &dir_entry : std::filesystem::directory_iterator(folderPath))
        {
            if (dir_entry.is_directory())
            {
                std::string weaponName = "";
                std::vector<std::filesystem::path> paths;
                for (const auto &file : std::filesystem::directory_iterator(dir_entry))
                {
                    std::string ext = file.path().extension().string();
                    if (ext == ".earshot")
                    {
                        weaponName = file.path().stem().string();
                    }
                    else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
                    {
                        paths.push_back(file.path());
                    }
                }

                if (weaponName != "")
                {
                    for (auto &e : paths)
                    {
                        WeaponSoundSystem::Register(e, weaponName);
                    }
                }
            }
        }
    }
}

void WeaponSoundSystem::Initialize()
{
    plugin::Events::initGameEvent += []
    {
        SearchWeaponSounds(MOD_DATA_PATH("audio/weapon/"));
        SearchWeaponSounds(PLUGIN_PATH((char *)"EarShot/"));
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4DFAC6, plugin::H_CALL, 0x4E6A3D, plugin::H_CALL>, plugin::PRIORITY_BEFORE, plugin::ArgPick4N<CAEWeaponAudioEntity *, 0, eWeaponType, 1, CPhysical *, 2, int, 3>, void(CAEWeaponAudioEntity *, eWeaponType, CPhysical *, int)>
        CAEWeaponAudioEntity_WeaponFireEvent;

    CAEWeaponAudioEntity_WeaponFireEvent += [](CAEWeaponAudioEntity *pAudioEnt, eWeaponType weaponType, CPhysical *entity, int)
    {
        if (pAudioEnt && pAudioEnt->m_pPed)
        {
            std::string *path = WeaponSoundSystem::FindAudio(weaponType, "shoot");
            if (path)
            {
                AudioMgr::PlayFileSound(*path, pAudioEnt->m_pPed);
            }
            plugin::patch::SetRaw(0x504F80, path ? (char *)"\xC2\x0C\x00" : (char *)"\x8B\x44\x24", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4DFAD6, plugin::H_CALL, 0x4E6A5B, plugin::H_CALL>, plugin::PRIORITY_BEFORE,
                          plugin::ArgPick4N<CAEWeaponAudioEntity *, 0, eWeaponType, 1, CPed *, 2, int, 3>,
                          void(CAEWeaponAudioEntity *, eWeaponType, CPed *, int)>
        CAEWeaponAudioEntity_WeaponFireReload;

    CAEWeaponAudioEntity_WeaponFireReload += [](CAEWeaponAudioEntity *pAudioEnt, eWeaponType weaponType, CPed *pPed, int unk)
    {
        if (pAudioEnt && pAudioEnt->m_pPed)
        {
            std::string *path = WeaponSoundSystem::FindAudio(weaponType, "reload");
            if (path)
            {
                if (unk != 0x93) // SKIP AE_WEAPON_RELOAD_B
                {
                    if (path)
                    {
                        AudioMgr::PlayFileSound(*path, pAudioEnt->m_pPed);
                    }
                }
            }
            plugin::patch::SetRaw(0x503690, path ? (char *)"\xC2\x0C\x00" : (char *)"\x51\x53\x55", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4DFAE6, plugin::H_CALL>, plugin::PRIORITY_BEFORE,
                          plugin::ArgPick4N<CAEWeaponAudioEntity *, 0, eWeaponType, 1, CPed *, 2, int, 3>,
                          void(CAEWeaponAudioEntity *, eWeaponType, CPed *, int)>
        CAEWeaponAudioEntity_WeaponProjectile;

    CAEWeaponAudioEntity_WeaponProjectile += [](CAEWeaponAudioEntity *pAudioEnt, eWeaponType weaponType, CPed *pPed, int unk)
    {
        if (pAudioEnt && pAudioEnt->m_pPed)
        {
            std::string *path = WeaponSoundSystem::FindAudio(weaponType, "projectile");
            if (path)
            {
                AudioMgr::PlayFileSound(*path, pAudioEnt->m_pPed);
            }
            plugin::patch::SetRaw(0x4DF060, path ? (char *)"\xC2\x0C\x00" : (char *)"\x8B\x44\x24", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4E2CA9, plugin::H_CALL>, plugin::PRIORITY_BEFORE,
                          plugin::ArgPick6N<CAEPedAudioEntity *, 0, int, 1, CPhysical *, 2, char, 3, float, 4, unsigned int, 5>,
                          void(CAEPedAudioEntity *, int, CPhysical *, char, float, unsigned int)>
        CAEPedAudioEntity_HitEvent;

    CAEPedAudioEntity_HitEvent += [](CAEPedAudioEntity *pAudioEnt, int, CPhysical *entity, char a, float b, unsigned int c)
    {
        if (pAudioEnt && pAudioEnt->m_pPed)
        {
            eWeaponType weaponType = pAudioEnt->m_pPed->m_aWeapons[pAudioEnt->m_pPed->m_nSelectedWepSlot].m_eWeaponType;
            std::string *path = WeaponSoundSystem::FindAudio(weaponType, "hit");
            if (path)
            {
                AudioMgr::PlayFileSound(*path, pAudioEnt->m_pPed);
            }
            plugin::patch::SetRaw(0x4E1CC0, path ? (char *)"\xC2\x14\x00" : (char *)"\x83\xEC\x14", 3);
        }
    };

    plugin::ThiscallEvent<plugin::AddressList<0x4E2C7C, plugin::H_CALL>, plugin::PRIORITY_BEFORE,
                          plugin::ArgPick4N<CAEPedAudioEntity *, 0, int, 1, int, 2, int, 3>, char(CAEPedAudioEntity *, int, int, int)>
        CAEPedAudioEntity_SwingEvent;

    CAEPedAudioEntity_SwingEvent += [](CAEPedAudioEntity *pAudioEnt, int a, int b, int c)
    {
        if (pAudioEnt && pAudioEnt->m_pPed)
        {
            eWeaponType weaponType = pAudioEnt->m_pPed->m_aWeapons[pAudioEnt->m_pPed->m_nSelectedWepSlot].m_eWeaponType;
            std::string *path = WeaponSoundSystem::FindAudio(weaponType, "swing");
            if (path)
            {
                AudioMgr::PlayFileSound(*path, pAudioEnt->m_pPed);
            }
            plugin::patch::SetRaw(0x4E1A40, path ? (char *)"\xC2\x0C\x00" : (char *)"\x83\xEC\x08", 3);
        }
    };
}