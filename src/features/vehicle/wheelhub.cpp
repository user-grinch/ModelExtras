#include "pch.h"
#include "wheelhub.h"
#include "modelinfomgr.h"

void WheelHub::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
    {
        const char* nodeName = GetFrameNodeName(pFrame);
        if (!nodeName) {
            return;
        }

        VehData& data = xData.Get(pVeh);
        
        switch (Hash::Get(nodeName)) 
        {
            case "wheel_rf_dummy"_h: data.m_pWRF = pFrame; break;
            case "wheel_rm_dummy"_h: data.m_pWRM = pFrame; break;
            case "wheel_rr_dummy"_h:
            case "wheel_rb_dummy"_h: data.m_pWRR = pFrame; break;
            case "wheel_lf_dummy"_h: data.m_pWLF = pFrame; break;
            case "wheel_lm_dummy"_h: data.m_pWLM = pFrame; break;
            case "wheel_lr_dummy"_h:
            case "wheel_lb_dummy"_h: data.m_pWLR = pFrame; break;
            
            case "hub_rf"_h: data.m_pHRF = pFrame; break;
            case "hub_rm"_h: data.m_pHRM = pFrame; break;
            case "hub_rb"_h:
            case "hub_rr"_h: data.m_pHRR = pFrame; break;
            case "hub_lf"_h: data.m_pHLF = pFrame; break;
            case "hub_lm"_h: data.m_pHLM = pFrame; break;
            case "hub_lb"_h:
            case "hub_lr"_h: data.m_pHLR = pFrame; break;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!pVeh || !pVeh->GetIsOnScreen()) {
            return;
        }

        VehData& data = xData.Get(pVeh);
        bool modified = false;

        auto updateRotation = [&](RwFrame* ori, RwFrame* tar, bool isLeft) 
        {
            if (ori && tar)
            {
                double oriRot = MatrixUtil::GetRotationZ(&ori->modelling);
                double tarRot = MatrixUtil::GetRotationZ(&tar->modelling);
                MatrixUtil::SetRotationZAbsolute(&tar->modelling, (oriRot - tarRot));
                tar->modelling.pos.z = ori->modelling.pos.z;
                pVeh->UpdateRwFrame();
            }
        };

        updateRotation(data.m_pWRF, data.m_pHRF, false);
        updateRotation(data.m_pWRM, data.m_pHRM, false);
        updateRotation(data.m_pWRR, data.m_pHRR, false);
        updateRotation(data.m_pWLF, data.m_pHLF, true);
        updateRotation(data.m_pWLM, data.m_pHLM, true);
        updateRotation(data.m_pWLR, data.m_pHLR, true);

        if (modified) {
            pVeh->UpdateRwFrame();
        }
    });
}