#pragma once
#include "../audiomgr.h"

class SoundEffects
{
private:
    static inline StreamHandle m_hReverse = NULL;
    struct VehData
    {
        float m_fBrakePressure = 0.0f;
        bool m_bEngineState = false;
        bool m_bIndicatorState = false;

        VehData(CVehicle *) {}
    };
    static inline VehicleExtendedData<VehData> vehData;

public:
    static void Initialize();
};