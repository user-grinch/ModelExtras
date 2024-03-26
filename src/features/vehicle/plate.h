#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class LicensePlateFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;
  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh, std::string& name);
    void Process(RwFrame* frame, CVehicle* pVeh, std::string& name);
};

extern LicensePlateFeature LicensePlate;