#pragma once

#include <string>
#include <vector>
#include <plugin.h>

using json = nlohmann::json;

struct VehicleDummyData {
    CVector pos;
    float diff;
};

class VehicleSirenData {
private:
    std::string Key;
    RwRGBA Color;
    float Size;
    std::vector<int> Pattern;
    int PatternCount;
    int State;
    uint64_t Time;
    json JsonData;
    std::vector<VehicleDummyData> Dummies;

public:
    VehicleSirenData(json data, CVehicle* vehicle, std::string key);

    void ResetSirenData();
    void FindNodesRecursive(RwFrame* frame, CVehicle* vehicle, bool bReSearch, bool bOnExtras);
};
