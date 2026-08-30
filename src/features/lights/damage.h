#pragma once
#include "pch.h"
#include "utils/util.h"

struct LightDamageState {
    bool isFrontLeftOk;
    bool isFrontRightOk;
    bool isRearLeftOk;
    bool isRearRightOk;
    bool isMiddleLeftOk;
    bool isMiddleRightOk;
    bool isFrontBumperOk;

    static LightDamageState Get(CVehicle* pControlVeh, CVehicle* pTowedVeh) {
        LightDamageState state;
        state.isFrontLeftOk = (!Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_LEFT) && !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_LEFT)) || pControlVeh->bSirenOrAlarm;
        state.isFrontRightOk = (!Util::IsLightDamaged(pControlVeh, eLights::LIGHT_FRONT_RIGHT) && !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_RIGHT)) || pControlVeh->bSirenOrAlarm;
        state.isRearLeftOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_LEFT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_LEFT));
        state.isRearRightOk = !(Util::IsLightDamaged(pTowedVeh, eLights::LIGHT_REAR_RIGHT) || Util::IsPanelDamaged(pTowedVeh, ePanels::WING_REAR_RIGHT));
        state.isMiddleLeftOk = !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_LEFT);
        state.isMiddleRightOk = !Util::IsPanelDamaged(pControlVeh, ePanels::WING_FRONT_RIGHT);
        state.isFrontBumperOk = !Util::IsPanelDamaged(pControlVeh, ePanels::BUMP_FRONT);
        return state;
    }
};
