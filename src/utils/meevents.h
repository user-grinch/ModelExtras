#pragma once
#include <Events.h>
#include <CModelInfo.h>
#include <Patch.h>

namespace MEEvents
{
    using namespace plugin;
    // vehicle
    static inline ThiscallEvent<AddressList<0x5343B2, H_CALL>, PRIORITY_AFTER, ArgPickN<CVehicle *, 0>, void(CVehicle *)> heliRenderEvent;
    static inline ThiscallEvent<AddressList<0x6D0E89, H_JUMP>, PRIORITY_BEFORE, ArgPickN<CVehicle *, 0>, void(CVehicle *)> vehRenderEvent;

    // Weapons
    static inline ThiscallEvent<AddressList< // 0x43D821, H_CALL,
                                                             //    0x43D939, H_CALL,
                                            0x45CC78, H_CALL,
                                            0x47D3AD, H_CALL,
                                            //   0x5E3B53, H_CALL,
                                            //   0x5E5F14, H_CALL,
                                            //   0x5E6150, H_CALL,
                                            //   0x5E6223, H_CALL,
                                            0x5E6327, H_CALL,
                                            0x63072E, H_CALL,
                                            //   0x5E6483, H_CALL,
                                            0x6348FC, H_CALL>,
                                        PRIORITY_BEFORE, ArgPick2N<CPed *, 0, int, 1>, void(CPed *, int)>
        weaponRemoveEvent;
    static inline CdeclEvent<AddressList<0x5E7859, H_CALL>, PRIORITY_BEFORE, ArgPickN<CPed *, 0>, void(CPed *)> weaponRenderEvent;
    static inline ThiscallEvent<AddressList<0x5E6192, H_CALL>, PRIORITY_BEFORE, ArgPick4N<CWeapon *, 0, int, 1, int, 2, CPed *, 3>, void(CWeapon *, int, int, CPed *)> weaponInitEvent;
    
    static inline ThiscallEvent <AddressList<0x5E6342, H_CALL>, PRIORITY_BEFORE, ArgPickN<CWeapon*, 0>, void(CWeapon*)>  weaponDtorEvent;
}