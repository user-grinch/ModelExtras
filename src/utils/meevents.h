#pragma once
#include <Events.h>
#include <game_sa/CModelInfo.h>
#include <Patch.h>

namespace MEEvents
{
    using namespace plugin;
    // vehicle
    static inline ThiscallEvent<AddressList<0x5343B2, H_CALL>, PRIORITY_AFTER, ArgPickN<CVehicle *, 0>, void(CVehicle *)> heliRenderEvent;
    static inline ThiscallEvent<AddressList<0x6D0E89, H_JUMP>, PRIORITY_BEFORE, ArgPickN<CVehicle *, 0>, void(CVehicle *)> vehRenderEvent;
    static inline ThiscallEvent<AddressList<0x6AAB71, H_CALL, 0x6BEA36, H_CALL>, PRIORITY_BEFORE, ArgPickN<CVehicle *, 0>, void(CVehicle *)> vehPreRenderEvent;
}