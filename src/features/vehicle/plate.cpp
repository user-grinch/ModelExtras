#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>

LicensePlateFeature LicensePlate;
void LicensePlateFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    CVehicleModelInfo *pVehModelInfo = static_cast<CVehicleModelInfo*>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
    pVehModelInfo->m_pPlateMaterial = 0;
    pVehModelInfo->m_szPlateText[0] = 0;
    pVehModelInfo->m_nPlateType = -1;
    auto t = CCustomCarPlateMgr::SetupClump(pVehModelInfo->m_pRwClump, (char*)"2111112\0", pVehModelInfo->m_nPlateType);

    if (t) {
        pVehModelInfo->m_pPlateMaterial = t;
    }
}

void LicensePlateFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        std::string name = GetFrameNodeName(frame);
        // if (name.find("x_plate") != std::string::npos)
        // {
            Initialize(frame, pVeh);
        // }
    }
}