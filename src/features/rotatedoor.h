#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct RotateDoorConfig {
        RwFrame* frame = nullptr;
        float originalRot = 0.0f;
        float mul = 1.0f;
        float popOutAmount = 0.0f;
        float prevRot = 0.0f;
    };

struct RotateDoorData {
    std::vector<RotateDoorConfig> leftFront;
    std::vector<RotateDoorConfig> rightFront;
    std::vector<RotateDoorConfig> leftRear;
    std::vector<RotateDoorConfig> rightRear;
    std::vector<RotateDoorConfig> boot;
    std::vector<RotateDoorConfig> bonnet;

    RotateDoorData(CVehicle* pVeh) {}
};

class RotateDoor : public CVehFeature<RotateDoorData>
{
protected:
    void Init() override;
    

    

    static void UpdateDoorGroup(CVehicle* pVeh, std::vector<RotateDoorConfig>& configs, eDoors doorID, bool isBootBonnet);
    static void UpdateSingleFrame(CVehicle* pVeh, RotateDoorConfig& config, eDoors doorID, bool isBootBonnet);

public:
    RotateDoor() : CVehFeature<RotateDoorData>("AnimatedDoors", "FEATURES", eFeatureMatrix::AnimatedDoors) {}
};