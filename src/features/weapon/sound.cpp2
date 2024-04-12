#include "pch.h"
#include "sound.h"

using oCAEWeaponAudioEntity_WeaponFire = void(__thiscall *)(eWeaponType weaponType, CPhysical *entity, int audioEventId);
using oCAEWeaponAudioEntity_WeaponReload = void(__thiscall *)(eWeaponType weaponType, CPhysical *entity, int audioEventId);
using oCAEPedAudioEntity_HandlePedHit = void(__thiscall *)(int a2, CPhysical *a3, unsigned __int8 a4, float a5, unsigned int a6);
using oCAEPedAudioEntity_HandlePedSwing = char(__thiscall *)(int a2, int a3, int a4);

static ThiscallEvent <AddressList<0x73647F, H_CALL>, PRIORITY_AFTER, ArgPick5N<CVehicle*, 0, CPed*, 1,
        eWeaponType, 2, float, 3, CVector, 4>, void(CVehicle*, CPed*, eWeaponType, float, CVector)> 
        hkCAEWeaponAudioEntity_WeaponFireEvent;

void __fastcall hkCAEWeaponAudioEntity_WeaponFire(CAEWeaponAudioEntity *ptr, void *unusedpointer, eWeaponType weaponType, CPhysical *entity, int audioEventId) {
    ptr->WeaponFire(weaponType, entity, audioEventId);
}

void __fastcall hkCAEWeaponAudioEntity_WeaponReload(CAEWeaponAudioEntity *ptr, void *unusedpointer, eWeaponType weaponType, CPhysical *entity, int audioEventId) {
    ptr->WeaponReload(weaponType, entity, audioEventId);
}

void __fastcall hkCAEPedAudioEntity_HandlePedHit(CAEPedAudioEntity *ptr, void *unusedpointer, int a2, CPhysical *a3, unsigned __int8 a4, float a5, unsigned int a6) {
    plugin::CallMethod<0x4E1CC0, CAEPedAudioEntity *, int, CPhysical *, uint8_t, float, uint>(ptr, a2, a3, a4, a5, a6);
}

char __fastcall hkCAEPedAudioEntity_HandlePedSwing(CAEPedAudioEntity *ptr, void *unusedpointer, int a2, int a3, int a4) {
    auto returnvalue = plugin::CallMethodAndReturn<char, 0x4E1A40, CAEPedAudioEntity *, int, int, int>(ptr, a2, a3, a4);
    return 1;
}

WeaponSoundFeature WeaponSound;

void WeaponSoundFeature::Initialize() {
    
}