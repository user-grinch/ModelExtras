#pragma once
#include "pch.h"
#include <string>
#include <map>

class VehiclePaintjobs {
private:
    static inline std::map<int, std::map<int, int>> modelPaintjobs;
    static inline std::map<int, bool> vehiclePaintjob;

public:
    static void Initialize();
    static void Read(int model, nlohmann::json json);
    static void OnVehicleSetModel(CVehicle* vehicle);
    static void OnVehicleRender(CVehicle* vehicle);
};