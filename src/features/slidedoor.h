#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct SlideDoorConfig {
        RwFrame* frame = nullptr;
        float mul = 1.0f;
        float popOutAmount = 0.2f;
    };

struct SlideDoorData {
    std::vector<SlideDoorConfig> leftFront;
    std::vector<SlideDoorConfig> rightFront;
    std::vector<SlideDoorConfig> leftRear;
    std::vector<SlideDoorConfig> rightRear;
    SlideDoorData(CVehicle *pVeh) {}
    ~SlideDoorData() {}
};

class SlideDoor : public CVehFeature<SlideDoorData>
{
protected:
    void Init() override;
    

    

    static void UpdateDoorGroup(CVehicle* pVeh, std::vector<SlideDoorConfig>& configs, eDoors doorID);

public:
    SlideDoor() : CVehFeature<SlideDoorData>("AnimatedDoors", "FEATURES", eFeatureMatrix::AnimatedDoors) {}
};