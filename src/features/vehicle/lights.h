#pragma once

#include <map>
#include "plugin.h"
#include "avs/dummy.h"
#include "avs/materials.h"

enum class VehicleLightState {
    Something = 0,
    LightLeft,
    LightRight,
    TailLight,
    Reverselight,
    Brakelight,
    Light,
    Daylight,
    Nightlight
};

class VehicleLights {
private:
    static inline std::map<int, std::map<VehicleLightState, std::vector<VehicleMaterial*>>> materials;
    static inline std::map<int, std::map<VehicleLightState, std::vector<VehicleDummy*>>> dummies;
    static inline std::map<int, bool> states;

    static void registerMaterial(CVehicle* vehicle, RpMaterial* material, VehicleLightState state);
    static void renderLights(CVehicle* vehicle, VehicleLightState state, float vehicleAngle, float cameraAngle);
    static void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
    static void enableMaterial(VehicleMaterial* material);

public:
    static void Initialize();
};