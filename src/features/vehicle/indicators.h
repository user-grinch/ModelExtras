#pragma once
#include <map>
#include "plugin.h"
#include "avs/dummy.h"
#include "avs/materials.h"

enum VehicleIndicatorState {
    Left = 0,
    Right,
    Both
};

class VehicleIndicators {
private:
    static inline std::map<int, std::map<VehicleIndicatorState, std::vector<RpMaterial*>>> materials;
    static inline std::map<int, std::map<VehicleIndicatorState, std::vector<VehicleDummy*>>> dummies;
    static inline std::map<int, VehicleIndicatorState> states;
    static inline uint64_t delay;
    static inline bool delayState;

    static void registerMaterial(CVehicle* vehicle, RpMaterial*& material, VehicleIndicatorState state);
    static void enableMaterial(RpMaterial* material);
    static void enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle);

public:
    static void Initialize();
};
