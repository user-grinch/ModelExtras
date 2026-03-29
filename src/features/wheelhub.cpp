#include "pch.h"
#include "wheelhub.h"
#include "utils/modelinfomgr.h"
#include "utils/util.h"

void WheelHub::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view name)
    {
        WheelHubData& data = m_VehData.Get(pVeh);
        
        if (name == "wheel_rf_dummy")      { data.m_pWRF = pFrame; }
        else if (name == "wheel_rm_dummy") { data.m_pWRM = pFrame; }
        else if (name == "wheel_rr_dummy" || name == "wheel_rb_dummy") { data.m_pWRR = pFrame; }
        else if (name == "wheel_lf_dummy") { data.m_pWLF = pFrame; }
        else if (name == "wheel_lm_dummy") { data.m_pWLM = pFrame; }
        else if (name == "wheel_lr_dummy" || name == "wheel_lb_dummy") { data.m_pWLR = pFrame; }

        else if (name == "hub_rf")         { data.m_pHRF = pFrame; }
        else if (name == "hub_rm")         { data.m_pHRM = pFrame; }
        else if (name == "hub_rr" || name == "hub_rb") { data.m_pHRR = pFrame; }
        else if (name == "hub_lf")         { data.m_pHLF = pFrame; }
        else if (name == "hub_lm")         { data.m_pHLM = pFrame; }
        else if (name == "hub_lr" || name == "hub_lb") { data.m_pHLR = pFrame; }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!pVeh || !pVeh->GetIsOnScreen()) {
            return;
        }

        WheelHubData& data = m_VehData.Get(pVeh);
        bool modified = false;
        
        // Thanks to Ameer & SanVive team for their rotation fix
        auto updateRotation = [&](RwFrame* ori, RwFrame* tar, bool isLeft) 
        {
            if (tar == nullptr) return;
            if (ori == nullptr) return;

            RwV3d rightVec = ori->modelling.right;
            if (isLeft) RwV3dNegate(&rightVec, &rightVec);

            MatrixUtil::ForceRightVector(&tar->modelling, rightVec);
            RwV3dNegate(&tar->modelling.up, &tar->modelling.up);

            tar->modelling.pos.z = ori->modelling.pos.z;

            pVeh->UpdateRwFrame();
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