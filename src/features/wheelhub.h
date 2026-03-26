#pragma once
#include <plugin.h>
#include "core/base.h"


struct WheelHubData {
    RwFrame *m_pWRF = nullptr, *m_pHRF = nullptr;
    RwFrame *m_pWRM = nullptr, *m_pHRM = nullptr;
    RwFrame *m_pWRR = nullptr, *m_pHRR = nullptr;
    RwFrame *m_pWLF = nullptr, *m_pHLF = nullptr;
    RwFrame *m_pWLM = nullptr, *m_pHLM = nullptr;
    RwFrame *m_pWLR = nullptr, *m_pHLR = nullptr;

    WheelHubData(CVehicle *pVeh) {}
    ~WheelHubData() {}
};

class WheelHub : public CVehFeature<WheelHubData>
{
protected:
    void Init() override;
    

public:
    WheelHub() : CVehFeature<WheelHubData>("RotatingWheelHubs", "FEATURES", eFeatureMatrix::RotatingWheelHubs) {}
};