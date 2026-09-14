#pragma once
#include "core/base.h"
#include "data.h"

class Lights : public CBaseFeature {
protected:
    void Init() override;

public:
    static inline bool m_bEnabled = false;
    Lights() : CBaseFeature("StandardLights", "LIGHTS", eFeatureMatrix::StandardLights) {}
    void ReloadConfig() override;
    void Reload(CVehicle* pVeh) override;

    bool HasProcessTick() const override { return true; }
    bool HasProcessVehicle() const override { return true; }
    bool HasProcessBikePointLights() const override { return true; }

    void ProcessTick() override;
    void ProcessVehicle(CVehicle* pVeh) override;
    void ProcessBikePointLights(CVehicle* pVeh) override;

    static VehLightData& GetVehicleData(CVehicle* pVeh);
    static bool IsIndicatorOn(CVehicle* pVeh);
    static bool GetLightState(CVehicle* pVeh, eMaterialType lightId);
    static void SetLightState(CVehicle* pVeh, eMaterialType lightId, bool state);
};

using LightsFeature = Lights;
