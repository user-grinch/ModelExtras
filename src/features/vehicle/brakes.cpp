#include "pch.h"
#include "Brakes.h"
#include "datamgr.h"
#include "modelinfomgr.h"

void FrontBrake::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame) {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_fbrake") || name.starts_with("fc_fbrake")) {
            VehData &data = vehData.Get(pVeh);
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData.contains("frontbrake") && jsonData["frontbrake"].contains("maxrotation"))
            {
                data.m_nMaxRotation = jsonData["frontbrake"].value("maxrotation", 360.0f);
            }
            data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nMaxRotation / 5));
            data.pFrame = pFrame;
            data.m_bInitialized = true;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle* pVeh) {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        auto& data = vehData.Get(pVeh);
        if (!data.m_bInitialized || !data.pFrame) return;

        uint timer = static_cast<int>(CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale);
        uint deltaTime = (timer - data.m_nLastFrameMS);

        if (deltaTime > data.m_nWaitTime)
        {
            float temp;
            if (pVeh->bIsHandbrakeOn && data.m_nCurRotation != data.m_nMaxRotation)
            {
                if (data.m_nMaxRotation < data.m_nCurRotation)
                {
                    data.m_nCurRotation -= 1;
                    temp = -1;
                }
                else
                {
                    data.m_nCurRotation += 1;
                    temp = 1;
                }

                FrameUtil::SetRotationZ(data.pFrame, temp);
                data.m_nLastFrameMS = timer;
            }
            else
            {
                if (data.m_nCurRotation != 0)
                {
                    if (data.m_nMaxRotation > 0)
                    {
                        data.m_nCurRotation -= 1;
                        temp = -1;
                    }
                    else
                    {
                        data.m_nCurRotation += 1;
                        temp = 1;
                    }

                    FrameUtil::SetRotationZ(data.pFrame, temp);
                    data.m_nLastFrameMS = timer;
                }
            }
        }
        
    });
}

void RearBrake::Initialize() {
    ModelInfoMgr::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame) {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_rbrake") || name.starts_with("fc_rbrake")) {
            VehData &data = vehData.Get(pVeh);
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData.contains("rearbrake") && jsonData["rearbrake"].contains("maxrotation"))
            {
                data.m_nMaxRotation = jsonData["rearbrake"].value("maxrotation", data.m_nMaxRotation);
            }
            data.m_nWaitTime = static_cast<unsigned int>(abs(data.m_nMaxRotation / 5));
            data.pFrame = pFrame;
            data.m_bInitialized = true;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle* pVeh) {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        auto& data = vehData.Get(pVeh);
        if (!data.m_bInitialized || !data.pFrame) return;

        uint timer = static_cast<int>(CTimer::m_snTimeInMilliseconds * CTimer::ms_fTimeScale);
        uint deltaTime = (timer - data.m_nLastFrameMS);

        if (deltaTime > data.m_nWaitTime)
        {
            float temp;
            if (pVeh->m_fBreakPedal && data.m_nCurRotation != data.m_nMaxRotation)
            {
                if (data.m_nMaxRotation < data.m_nCurRotation)
                {
                    data.m_nCurRotation -= 1;
                    temp = -1;
                }
                else
                {
                    data.m_nCurRotation += 1;
                    temp = 1;
                }

                FrameUtil::SetRotationX(data.pFrame, temp);
                data.m_nLastFrameMS = timer;
            }
            else
            {
                if (data.m_nCurRotation != 0)
                {
                    if (data.m_nMaxRotation > 0)
                    {
                        data.m_nCurRotation -= 1;
                        temp = -1;
                    }
                    else
                    {
                        data.m_nCurRotation += 1;
                        temp = 1;
                    }

                    FrameUtil::SetRotationX(data.pFrame, temp);
                    data.m_nLastFrameMS = timer;
                }
            }
        }
    });
}