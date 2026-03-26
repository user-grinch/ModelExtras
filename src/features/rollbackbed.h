#pragma once
#include <plugin.h>
#include "core/base.h"
#include <vector>

struct HydraulicPiston
    {
        RwFrame* pFrame = nullptr;
        float fCurMove = 0.0f;
        float fLastMove = 0.0f;
        float fTargetMove = 2.0f; // Unique target distance for this piston
    };

struct RollbackBedData
{
    bool bInit = false;
    bool bExpanded = false;

    RwFrame *pBedFrame = nullptr;
    float fBedCurRot = 0.0f;
    float fBedTargetRot = 0.0f;
    float fBedRotSpeed = 1.0f;

    RwFrame *pHydralicsShellFrame = nullptr;
    float fHyCurRot = 0.0f;
    float fHyTargetRot = 0.0f;
    float fHyRotSpeed = 1.0f;

    std::vector<HydraulicPiston> m_Pistons;
    float fGlobalMoveSpeed = 1.0f;

    RollbackBedData(CVehicle *pVeh) {}
    ~RollbackBedData() {}
};

class RollbackBed : public CVehFeature<RollbackBedData>
{
protected:
    void Init() override;
    // Struct to hold state for EACH piston individually
    

    
    static bool UpdateRotation(CVehicle *pVe, RwFrame *pFrame, float targetRot, float &curRot, float speed);
    
    // Updated signature
    static bool UpdateMove(HydraulicPiston &piston, float moveSpeed, bool bExpanded);

public:
    RollbackBed() : CVehFeature<RollbackBedData>("RollbackBed", "FEATURES", eFeatureMatrix::RollbackBed) {}
};