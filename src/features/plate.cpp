#include "pch.h"
#include "plate.h"

static int SetCarCustomPlate(CVehicleModelInfo *pInfo) {
  char buf[12];

  pInfo->m_pPlateMaterial = 0;
  pInfo->m_szPlateText[0] = 0;
  pInfo->m_nPlateType = -1;
  strcpy(buf, "2111112");
  buf[8] = 0;
  
  patch::Set<BYTE>(0xC3EF80, 0);
  patch::Set<BYTE>(0xC3EF7C, 0);
  RpClumpForAllAtomics(pInfo->m_pRwClump, reinterpret_cast<RpAtomicCallBack>(0x6FE0D0), buf);

  int result = patch::Get<BYTE>(0xC3EF7C);
  if (result) {
    pInfo->m_pPlateMaterial = reinterpret_cast<RpMaterial*>(result);
  }
  return result;
}

LicensePlateFeature LicensePlate;
void LicensePlateFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    SetCarCustomPlate(static_cast<CVehicleModelInfo*>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex)));
}

void LicensePlateFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    if (!data.m_bInitialized) {
        std::string name = GetFrameNodeName(frame);
        // if (name.find("vx_plate") != std::string::npos)
        // {
            Initialize(frame, pVeh);
        // }
    }
}