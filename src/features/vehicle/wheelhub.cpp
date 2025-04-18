#include "pch.h"
#include "wheelhub.h"
#include <CCutsceneMgr.h>
#include "avs/materials.h"

void UpdateRotation(RwFrame *ori, RwFrame *tar)
{
    if (ori && tar)
    {
        float oriRot = Util::GetMatrixRotationZ(&ori->modelling);
        Util::SetMatrixRotationZ(&tar->modelling, oriRot);
    }
}

void WheelHub::Initialize()
{
    VehicleMaterials::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, std::string name, bool parent)
                                    {
        VehData& data = xData.Get(pVeh);
        if (name == "wheel_rf_dummy") data.m_pWRF = pFrame;
        else if (name == "wheel_rm_dummy") data.m_pWRM = pFrame;
        else if (name == "wheel_rr_dummy") data.m_pWRR = pFrame;
        else if (name == "wheel_lf_dummy") data.m_pWLF = pFrame;
        else if (name == "wheel_lm_dummy") data.m_pWLM = pFrame;
        else if (name == "wheel_lr_dummy") data.m_pWLR = pFrame;
        else if (name == "hub_rf") data.m_pHRF = pFrame;
        else if (name == "hub_rm") data.m_pHRM = pFrame;
        else if (name == "hub_rb") data.m_pHRR = pFrame;
        else if (name == "hub_lf") data.m_pHLF = pFrame;
        else if (name == "hub_lm") data.m_pHLM = pFrame;
        else if (name == "hub_lb") data.m_pHLR = pFrame; });

    VehicleMaterials::RegisterRender([](CVehicle *pVeh)
                                     {
        if (!pVeh || !pVeh->GetIsOnScreen() || CCutsceneMgr::ms_running)
        {
            return;
        }

        VehData& data = xData.Get(pVeh);
        UpdateRotation(data.m_pWRF, data.m_pHRF);
        UpdateRotation(data.m_pWRM, data.m_pHRM);
        UpdateRotation(data.m_pWRR, data.m_pHRR);
        UpdateRotation(data.m_pWLF, data.m_pHLF);
        UpdateRotation(data.m_pWLM, data.m_pHLM);
        UpdateRotation(data.m_pWLR, data.m_pHLR); });
}