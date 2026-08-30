#pragma once
#include "enums/materialtype.h"
#include "enums/indicatorstate.h"
#include "core/dummy.h"
#include "core/colors.h"
#include <array>
#include <vector>
#include <string>
#include <algorithm>

struct LightsConfig {
    static LightsConfig& Get() {
        static LightsConfig instance;
        return instance;
    }

    bool gbGlobalIndicatorLights = false;
    bool gbLightCoronasFeature = false;
    bool gbLightPointLights = true;
    bool gbSirenPointLights = true;

    float gfGlobalCoronaSize = 0.3f;
    int gGlobalCoronaIntensity = 250;
    int gGlobalShadowIntensity = 220;
    float gfTailLightCoronaSize = 0.8f;
    int gTailLightCoronaIntensity = 60;
    float headlightSz = 5.0f;

    void InitConfig() {
        gbGlobalIndicatorLights = gConfig.ReadBoolean("LIGHTS", "StandardLights_GlobalIndicatorLights", gConfig.ReadBoolean("FEATURES", "StandardLights_GlobalIndicatorLights", false));
        gbLightCoronasFeature = gConfig.ReadBoolean("LIGHTS", "LightCoronas", gConfig.ReadBoolean("FEATURES", "LightCoronas", false));
        gbLightPointLights = gConfig.ReadBoolean("LIGHTS", "PointLights", gConfig.ReadBoolean("LIGHTS", "LightPointLights", gConfig.ReadBoolean("FEATURES", "PointLights", true)));
        gbSirenPointLights = gConfig.ReadBoolean("LIGHTS", "SirenPointLights", gConfig.ReadBoolean("FEATURES", "SirenPointLights", true));

        gfGlobalCoronaSize = gConfig.ReadFloat("LIGHTS", "LightCoronaSize", gConfig.ReadFloat("VISUAL", "LightCoronaSize", 0.3f));
        gGlobalShadowIntensity = gConfig.ReadInteger("LIGHTS", "LightShadowIntensity", gConfig.ReadInteger("VISUAL", "LightShadowIntensity", 220));
        gGlobalCoronaIntensity = gConfig.ReadInteger("LIGHTS", "LightCoronaIntensity", gConfig.ReadInteger("VISUAL", "LightCoronaIntensity", 250));
        gfTailLightCoronaSize = gConfig.ReadFloat("LIGHTS", "TailLightCoronaSize", gConfig.ReadFloat("VISUAL", "TailLightCoronaSize", 0.8f));
        gTailLightCoronaIntensity = gConfig.ReadInteger("LIGHTS", "TailLightCoronaIntensity", gConfig.ReadInteger("VISUAL", "TailLightCoronaIntensity", 60));
    }

private:
    LightsConfig() = default;
};

struct BlinkerState {
    static BlinkerState& Get() {
        static BlinkerState instance;
        return instance;
    }

    bool bIndicatorsDelay = false;
    uint64_t nDelayTimer = 0;

    void Reset() {
        nDelayTimer = 0;
        bIndicatorsDelay = false;
    }

    void Update() {
        size_t timestamp = CTimer::m_snTimeInMilliseconds;
        if ((timestamp - nDelayTimer) > 500) {
            nDelayTimer = timestamp;
            bIndicatorsDelay = !bIndicatorsDelay;
        }
    }
};

using LightsGlobal = LightsConfig;

struct VehLightData {
    bool bFogLightsOn = false;
    bool bLongLightsOn = false;
    eIndicatorState nIndicatorState = eIndicatorState::Off;
    bool bUsingGlobalIndicators = false;
    bool bWasAutoSteerActive = false;
    
    std::array<std::vector<VehicleDummy*>, eMaterialType::TotalMaterial> dummies;
    
    bool bLightStates[eMaterialType::TotalMaterial];
    unsigned int nHeadlightTickFrame = 0;

    VehLightData(CVehicle* pVeh = nullptr) {
        std::fill(std::begin(bLightStates), std::end(bLightStates), true);
    }

    VehLightData(const VehLightData&) = delete;
    VehLightData& operator=(const VehLightData&) = delete;

    VehLightData(VehLightData&& other) noexcept {
        *this = std::move(other);
    }

    VehLightData& operator=(VehLightData&& other) noexcept {
        if (this != &other) {
            ClearDummies();
            bFogLightsOn = other.bFogLightsOn;
            bLongLightsOn = other.bLongLightsOn;
            nIndicatorState = other.nIndicatorState;
            bUsingGlobalIndicators = other.bUsingGlobalIndicators;
            bWasAutoSteerActive = other.bWasAutoSteerActive;
            dummies = std::move(other.dummies);
            for (auto& vec : other.dummies) {
                vec.clear();
            }
            std::copy(std::begin(other.bLightStates), std::end(other.bLightStates), std::begin(bLightStates));
            nHeadlightTickFrame = other.nHeadlightTickFrame;
        }
        return *this;
    }

    void ClearDummies() {
        for (auto& vec : dummies) {
            for (auto* dummy : vec) {
                delete dummy;
            }
            vec.clear();
        }
    }
    
    ~VehLightData() {
        ClearDummies();
    }
};
