#pragma once
#include <plugin.h>

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

class ExtraWheel
{
protected:
    struct VehData
    {
        std::vector<std::vector<RwFrame *>> pExtras;
        std::vector<std::vector<RwFrame *>> pOriginals;

        VehData(CVehicle *pVeh)
        {
            pOriginals.resize(static_cast<int>(eWheelPos::COUNT));
            pExtras.resize(static_cast<int>(eWheelPos::COUNT));
        }
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;

public:
    static void Initialize();
};