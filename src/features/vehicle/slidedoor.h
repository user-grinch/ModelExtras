#pragma once
#include <plugin.h>
#include <vector>

class SlideDoor
{
protected:
    struct DoorConfig {
        RwFrame* frame = nullptr;
        float mul = 1.0f;
        float popOutAmount = 0.2f;
    };

    struct VehData {
        std::vector<DoorConfig> leftFront, rightFront;
        std::vector<DoorConfig> leftRear, rightRear;

        VehData(CVehicle* pVeh) {}
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;

    static void UpdateDoorGroup(CVehicle* pVeh, std::vector<DoorConfig>& configs, eDoors doorID);

public:
    static void Initialize();
};