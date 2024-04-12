#include "pch.h"
#include "chain.h"

ChainFeature Chain;

void ChainFeature::Initialize(RwFrame* pFrame, CVehicle* pVeh) {
    VehData &data = vehData.Get(pVeh);
    Util::StoreChilds(pFrame, data.m_FrameList);

}

void ChainFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    std::string name = GetFrameNodeName(frame);
    if (NODE_FOUND(name, "x_chain") || NODE_FOUND(name, "fc_chain")) {
        VehData &data = vehData.Get(pVeh);
        if (!data.m_bInitialized) {
            Initialize(frame, pVeh);
            data.m_bInitialized = true;
        }
        uint timer = static_cast<int>(CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale);
        uint deltaTime = timer - data.m_nLastFrameMS;

        float speed = Util::GetVehicleSpeedRealistic(pVeh);
        uint waitTime = static_cast<uint>(abs(speed)*0.01);

        if (deltaTime > waitTime) {
            if (pVeh->m_nVehicleSubClass == VEHICLE_BMX) {
                // Only move chain forward when pedal is rotating
                if (pVeh->m_fGasPedal && speed > 0) {
                    if (data.m_nCurChain == 0)
                        data.m_nCurChain = static_cast<short>(data.m_FrameList.size() - 1);
                    else
                        data.m_nCurChain -= 1;
                }
            } else {
                if (speed > 0) {
                    if (data.m_nCurChain == 0)
                        data.m_nCurChain = static_cast<short>(data.m_FrameList.size() - 1);
                    else
                        data.m_nCurChain -= 1;
                }

                if (speed < 0) {
                    if (data.m_nCurChain == data.m_FrameList.size() - 1)
                        data.m_nCurChain = 0;
                    else
                        data.m_nCurChain += 1;
                }
            }
            Util::HideAllChilds(frame);
            Util::ShowAllAtomics(data.m_FrameList[data.m_nCurChain]);
            data.m_nLastFrameMS = timer;
        }
    }
}