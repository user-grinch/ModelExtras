#pragma once
#include <Events.h>
#include <game_sa/CModelInfo.h>
#include <Patch.h>

namespace MEEvents
{
    using namespace plugin;
    // vehicle
    static inline ThiscallEvent<AddressList<0x6C4523, H_CALL>, PRIORITY_AFTER, ArgPickN<CVehicle *, 0>, void(CVehicle *)> heliRenderEvent;
    static inline ThiscallEvent<AddressList<0x6D0E89, H_JUMP>, PRIORITY_BEFORE, ArgPickN<CVehicle *, 0>, void(CVehicle *)> vehRenderEvent;
    // CAutomobile::PreRender (0x6AAB8B, after UpdateRwFrame) and CBike::PreRender (0x6BD3A8, after CalculateLeanMatrix)
    static inline ThiscallEvent<AddressList<0x6AAB8B, H_CALL, 0x6BD3A8, H_CALL>, PRIORITY_AFTER, ArgPickN<CVehicle *, 0>, void(CVehicle *)> vehPreRenderEvent;
}