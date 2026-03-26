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
}