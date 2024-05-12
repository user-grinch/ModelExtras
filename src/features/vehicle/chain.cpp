#include "pch.h"
#include "chain.h"


void Chain::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle *pVeh = (CVehicle*)ptr;
    VehData &data = vehData.Get(pVeh);

    if (!data.m_bInitialized) {
        VehData &data = vehData.Get((CVehicle*)pVeh);
        Util::StoreChilds(frame, data.m_FrameList);
        data.m_bInitialized = true;
    }
    
    float speed = Util::GetVehicleSpeedRealistic(pVeh);
    if (pVeh->m_nVehicleSubClass == VEHICLE_BMX) {
        // Only move chain forward when pedal is rotating
        if (pVeh->m_fGasPedal && speed > 0) {
            if (data.m_nCurChain == 0)
                data.m_nCurChain = static_cast<short>(data.m_FrameList.size() - 1);
            else
                data.m_nCurChain -= 1;
        }
    } else {
        if (speed > 0.5) {
            if (data.m_nCurChain == 0)
                data.m_nCurChain = static_cast<short>(data.m_FrameList.size() - 1);
            else
                data.m_nCurChain -= 1;
        }

        if (speed < -0.5) {
            if (data.m_nCurChain == data.m_FrameList.size() - 1)
                data.m_nCurChain = 0;
            else
                data.m_nCurChain += 1;
        }
    }
    Util::HideAllChilds(frame);
    Util::ShowAllAtomics(data.m_FrameList[data.m_nCurChain]);
}