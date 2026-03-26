#pragma once
#include <plugin.h>
#include <vector>

class RotateDoor
{
protected:
    struct DoorConfig {
        RwFrame* frame = nullptr;
        float originalRot = 0.0f;
        float mul = 1.0f;
        float popOutAmount = 0.0f;
        float prevRot = 0.0f;
    };

    struct VehData {
        std::vector<DoorConfig> leftFront, rightFront;
        std::vector<DoorConfig> leftRear, rightRear;
        std::vector<DoorConfig> boot, bonnet;

        VehData(CVehicle* pVeh) {}
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;

    static void UpdateDoorGroup(CVehicle* pVeh, std::vector<DoorConfig>& configs, eDoors doorID, bool isBootBonnet);
    static void UpdateSingleFrame(CVehicle* pVeh, DoorConfig& config, eDoors doorID, bool isBootBonnet);

public:
    static void Initialize();
};