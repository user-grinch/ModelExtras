#pragma once
#include <CModelInfo.h>
#include <Events.h>
#include <Patch.h>

namespace MEEvents {
// vehicle
static inline plugin::ThiscallEvent<
    plugin::AddressList<0x5343B2, plugin::H_CALL>, plugin::PRIORITY_AFTER,
    plugin::ArgPickN<CVehicle *, 0>, void(CVehicle *)>
    heliRenderEvent;
static inline plugin::ThiscallEvent<
    plugin::AddressList<0x6D0E89, plugin::H_JUMP>, plugin::PRIORITY_BEFORE,
    plugin::ArgPickN<CVehicle *, 0>, void(CVehicle *)>
    vehRenderEvent;

// Weapons
static inline plugin::ThiscallEvent<
    plugin::AddressList< // 0x43D821, H_CALL,
                         //    0x43D939, H_CALL,
        0x45CC78, plugin::H_CALL, 0x47D3AD, plugin::H_CALL,
        //   0x5E3B53, H_CALL,
        //   0x5E5F14, H_CALL,
        //   0x5E6150, H_CALL,
        //   0x5E6223, H_CALL,
        0x5E6327, plugin::H_CALL, 0x63072E, plugin::H_CALL,
        //   0x5E6483, H_CALL,
        0x6348FC, plugin::H_CALL>,
    plugin::PRIORITY_BEFORE, plugin::ArgPick2N<CPed *, 0, int, 1>,
    void(CPed *, int)>
    weaponRemoveEvent;
static inline plugin::CdeclEvent<plugin::AddressList<0x5E7859, plugin::H_CALL>,
                                 plugin::PRIORITY_BEFORE,
                                 plugin::ArgPickN<CPed *, 0>, void(CPed *)>
    weaponRenderEvent;
static inline plugin::ThiscallEvent<
    plugin::AddressList<0x5E6192, plugin::H_CALL>, plugin::PRIORITY_BEFORE,
    plugin::ArgPick4N<CWeapon *, 0, int, 1, int, 2, CPed *, 3>,
    void(CWeapon *, int, int, CPed *)>
    weaponInitEvent;
} // namespace MEEvents