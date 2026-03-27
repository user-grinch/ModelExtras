#pragma once
#include "enums/materialtype.h"
#include "core/dummy.h"
#include <functional>

class LightsGlobal {
public:
    static LightsGlobal& Get() {
        static LightsGlobal instance;
        return instance;
    }

    bool gbGlobalIndicatorLights = false;
    float gfGlobalCoronaSize = 0.3f;
    int gGlobalCoronaIntensity = 80;
    int gGlobalShadowIntensity = 80;
    float headlightSz = 5.0f;
    bool m_bLightStates[eMaterialType::TotalMaterial];

private:
    LightsGlobal() {
        std::fill(std::begin(m_bLightStates), std::end(m_bLightStates), true);
    }
    LightsGlobal(const LightsGlobal&) = delete;

    bool IsLightEnabled(eMaterialType type) {
        return m_bLightStates[type];
    }
};

struct LightsCommonData {
    bool bIsOn = false;
    DummyConfig config;
    std::map<eMaterialType, std::vector<VehicleDummy*>> dummies;
    std::function<bool(CVehicle*, eMaterialType)> shouldRender;

    LightsCommonData(CVehicle* pVeh) {
    }
};