#pragma once
#include <map>
#include <plugin.h>
#include "avs/dummy.h"
#include "avs/materials.h"

enum class VehicleFoglightState {
    FoglightLeft,
    FoglightRight
};

class VehicleFoglights {
private:
    static inline std::map<int, std::map<VehicleFoglightState, std::vector<VehicleMaterial*>>> materials;
    static inline std::map<int, std::map<VehicleFoglightState, std::vector<VehicleDummy*>>> dummies;
    static inline std::map<int, bool> states;

    static void registerMaterial(CVehicle* vehicle, RpMaterial* material, VehicleFoglightState state);
    static void enableDummy(VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);
    static void enableMaterial(VehicleMaterial* material);

public:
    static void Initialize();
};
