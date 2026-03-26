#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct RoofConfig {
      RwFrame* pFrame = nullptr;
      float speed = 1.0f;
      float currentRot = 0.0f;
      float prevRot = 0.0f;
      float targetRot = 30.0f;
    };

enum class AnimPhase {
        Idle,
        OpeningBoots,
        MovingRoof,
        ClosingBoots
    };struct RoofData {
    bool m_bInit = false;
    bool m_bRoofTargetExpanded = false;
    bool m_bPrevTarget = false;
    AnimPhase m_phase = AnimPhase::Idle;

    std::vector<RoofConfig> m_Boots;
    std::vector<RoofConfig> m_Roofs;

    RoofData(CVehicle *pVeh) {}
    ~RoofData() {}
};

class ConvertibleRoof : public CVehFeature<RoofData>
{
protected:
    void Init() override;
    

    

    
    static bool UpdateRotation(RoofConfig& config, CVehicle *pVeh, bool closed);
public:
    ConvertibleRoof() : CVehFeature<RoofData>("ConvertibleRoof", "FEATURES", eFeatureMatrix::ConvertibleRoof) {}

    static bool IsRoofOpen(CVehicle *pVeh) {
        return m_VehData.Get(pVeh).m_bRoofTargetExpanded;
    }
};
