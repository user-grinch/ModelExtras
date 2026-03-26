#pragma once
#include <plugin.h>
#include <vector>

class DigitalClockFeature
{
protected:
    struct VehData
    {
        RwFrame *m_pRootFrame = nullptr;
        RwFrame *m_pDigitsRoot = nullptr;
        RwFrame *m_pDigitPos[4] = {nullptr};
        std::vector<RwFrame *> m_DigitList;
        bool m_b12HourFormat = false;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> vehData;

public:
    static void Initialize();
};