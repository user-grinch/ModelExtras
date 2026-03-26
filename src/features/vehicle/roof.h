#pragma once
#include <plugin.h>
#include <vector>

class ConvertibleRoof {
protected:
    enum class AnimPhase {
        Idle,
        OpeningBoots,
        MovingRoof,
        ClosingBoots
    };

    struct RoofConfig {
      RwFrame* pFrame = nullptr;
      float speed = 1.0f;
      float currentRot = 0.0f;
      float prevRot = 0.0f;
      float targetRot = 30.0f;
    };

    struct VehData {
        bool m_bInit = false;
        std::vector<RoofConfig> m_Roofs, m_Boots;    
        bool m_bRoofTargetExpanded = true;
        bool m_bPrevTarget = true;
        AnimPhase m_phase = AnimPhase::Idle;

        VehData(CVehicle* pVeh) {
            m_bRoofTargetExpanded = RandomNumberInRange(0, 1);
        }
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;
    static bool UpdateRotation(RoofConfig& config, CVehicle *pVeh, bool closed);
public:
    static void Initialize();

    static bool IsRoofOpen(CVehicle *pVeh) {
        return xData.Get(pVeh).m_bRoofTargetExpanded;
    }
};
