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
    // CAutomobile::PreRender only. Bikes go through the processScriptsEvent loop in each
    // feature instead, CBike::PreRender has no equivalent call site to hook here.
    static inline ThiscallEvent<AddressList<0x6AAB71, H_CALL>, PRIORITY_BEFORE, ArgPickN<CVehicle *, 0>, void(CVehicle *)> vehPreRenderEvent;
}