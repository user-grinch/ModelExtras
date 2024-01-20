#pragma once
#include "plugin.h"
#include "../../interface/ifeature.hpp"
#include <vector>

class GearMeterFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;
        uint m_nCurrent = 0;
        std::vector<RwFrame*> m_FrameList;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
    void Process(RwFrame* frame, CVehicle* pVeh);
};

extern GearMeterFeature GearMeter;

class OdoMeterFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;
        int m_nTempVal = 0;
        std::string m_ScreenText = "000000";
        std::vector<RwFrame*> m_FrameList;
        float m_fMul = 160.9f;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
    void Process(RwFrame* frame, CVehicle* pVeh);
};

extern OdoMeterFeature OdoMeter;

class RpmMeterFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;
        int m_nMaxRpm = 0;
        float m_fCurRotation = 0.0f;
        float m_fMaxRotation = 0.0f;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
    void Process(RwFrame* frame, CVehicle* pVeh);
};

extern RpmMeterFeature RpmMeter;

class SpeedMeterFeature : public IFeature {
  protected:
    struct VehData {
        bool m_bInitialized = false;
        int m_nMaxSpeed = 0;
        float m_fMul = 160.9f;
        float m_fCurRotation = 0.0f;
        float m_fMaxRotation = 0.0f;

        VehData(CVehicle *pVeh) {}
        ~VehData() {}
    };

    VehicleExtendedData<VehData> vehData;

  public:
    void Initialize(RwFrame* frame, CVehicle* pVeh);
    void Process(RwFrame* frame, CVehicle* pVeh);
};

extern SpeedMeterFeature SpeedMeter;