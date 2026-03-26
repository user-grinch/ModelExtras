#pragma once
#include <plugin.h>
#include "core/base.h"

enum class eWheelPos
{
    LeftFront,
    RightFront,
    LeftMiddle,
    RightMiddle,
    LeftRear,
    RightRear,
    COUNT,
};

struct ExtraWheelData
{
    std::vector<RwFrame*> pOriginals[static_cast<int>(eWheelPos::COUNT)];
    std::vector<RwFrame*> pExtras[static_cast<int>(eWheelPos::COUNT)];

    ExtraWheelData(CVehicle *pVeh) {}
    ~ExtraWheelData() {}
};

class ExtraWheel : public CVehFeature<ExtraWheelData>
{
protected:
    void Init() override;
    

public:
    ExtraWheel() : CVehFeature<ExtraWheelData>("ExtraWheels", "FEATURES", eFeatureMatrix::ExtraWheels) {}
};