#pragma once
#include "enums/materialtype.h"
#include "enums/indicatorstate.h"
#include "core/dummy.h"
#include "core/colors.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>

struct LightsGlobal {
    static LightsGlobal& Get() {
        static LightsGlobal instance;
        return instance;
    }

    bool gbGlobalIndicatorLights = false;
    float gfGlobalCoronaSize = 0.3f;
    int gGlobalCoronaIntensity = 80;
    int gGlobalShadowIntensity = 80;
    float headlightSz = 5.0f;
    bool bIndicatorsDelay = false;
    uint64_t nDelayTimer = 0;

private:
    LightsGlobal() = default;
};

struct VehLightData {
    bool bFogLightsOn = false;
    bool bLongLightsOn = false;
    eIndicatorState nIndicatorState = eIndicatorState::Off;
    bool bUsingGlobalIndicators = false;
    
    // Maps material types to their associated dummies
    std::map<eMaterialType, std::vector<VehicleDummy*>> dummies;
    
    // Cache for material availability if needed
    bool bLightStates[eMaterialType::TotalMaterial];

    VehLightData(CVehicle* pVeh) {
        std::fill(std::begin(bLightStates), std::end(bLightStates), true);
    }
    
    ~VehLightData() {
        for (auto& pair : dummies) {
            for (auto* dummy : pair.second) {
                delete dummy;
            }
        }
    }
};
