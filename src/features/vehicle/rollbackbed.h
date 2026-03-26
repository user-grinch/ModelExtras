#pragma once
#include <plugin.h>
#include <vector>

class RollbackBed
{
protected:
    // Struct to hold state for EACH piston individually
    struct HydraulicPiston
    {
        RwFrame* pFrame = nullptr;
        float fCurMove = 0.0f;
        float fLastMove = 0.0f;
        float fTargetMove = 2.0f; // Unique target distance for this piston
    };

    struct VehData
    {
        bool bInit = false;
        RwFrame *pBedFrame = nullptr;
        RwFrame *pHydralicsShellFrame = nullptr;
        
        // List of all hydraulic pistons
        std::vector<HydraulicPiston> m_Pistons;

        float fBedTargetRot = 30.0f;
        float fBedCurRot = 0.0f;
        float fBedRotSpeed = 1.0f;

        float fHyTargetRot = 30.0f;
        float fHyCurRot = 0.0f;
        float fHyRotSpeed = 1.0f;

        // Global speed setting for hydraulics
        float fGlobalMoveSpeed = 1.0f; 

        bool bExpanded = false;
        VehData(CVehicle *pVeh)
        {
        }
        ~VehData() {}
    };

    static inline VehicleExtendedData<VehData> xData;
    static bool UpdateRotation(CVehicle *pVe, RwFrame *pFrame, float targetRot, float &curRot, float speed);
    
    // Updated signature
    static bool UpdateMove(HydraulicPiston &piston, float moveSpeed, bool bExpanded);

public:
    static void Initialize();
};