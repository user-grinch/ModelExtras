#pragma once
#include "pch.h"
#include "utils/util.h"

struct LightDamageState {
    bool isHeadlightLeftOk;
    bool isHeadlightRightOk;
    bool isFrontLeftOk;
    bool isFrontRightOk;
    bool isRearLeftOk;
    bool isRearRightOk;
    bool isMiddleLeftOk;
    bool isMiddleRightOk;
    static LightDamageState Get(CVehicle* pVeh) {
        LightDamageState state{};
        if (!pVeh) return state;
        state.isHeadlightLeftOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT) && !Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT);
        state.isHeadlightRightOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT) && !Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT);
        state.isFrontLeftOk = state.isHeadlightLeftOk || pVeh->bSirenOrAlarm;
        state.isFrontRightOk = state.isHeadlightRightOk || pVeh->bSirenOrAlarm;
        state.isRearLeftOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_LEFT) && !Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_LEFT);
        state.isRearRightOk = !Util::IsLightDamaged(pVeh, eLights::LIGHT_REAR_RIGHT) && !Util::IsPanelDamaged(pVeh, ePanels::WING_REAR_RIGHT);
        state.isMiddleLeftOk = !Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_LEFT);
        state.isMiddleRightOk = !Util::IsPanelDamaged(pVeh, ePanels::WING_FRONT_RIGHT);
        return state;
    }

    static LightDamageState Get(CVehicle* pControlVeh, CVehicle* /*pTowedVeh*/) {
        return Get(pControlVeh);
    }
};
