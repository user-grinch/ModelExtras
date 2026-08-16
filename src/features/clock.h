#pragma once
#include "core/base.h"

struct ClockData
{
    RwFrame *m_pDigitsRoot = nullptr;
    RwFrame *m_pDigitPos[4] = {nullptr, nullptr, nullptr, nullptr};
    bool m_b12HourFormat = false;

    ClockData(CVehicle *pVeh) {}
    ~ClockData() {}
};

class DigitalClockFeature : public CVehFeature<ClockData>
{
protected:
    void Init() override;

public:
    DigitalClockFeature() : CVehFeature<ClockData>("DigitalClock", "FEATURES", eFeatureMatrix::Clock) {}
};