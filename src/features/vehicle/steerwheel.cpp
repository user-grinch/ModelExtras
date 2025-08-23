#include "pch.h"
#include "steerwheel.h"
#include "modelinfomgr.h"
#define ROTATION_VAL 90.0f

void SteerWheel::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                               {
        auto& data = xData.Get(pVeh);

        // VehFuncs
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("f_steer")) {
            if (name.length() >= 7 && isdigit(name[7]))
            {
                data.factor = (float)std::stoi(&name[7]) / 2;
            }
            else
            {
                data.factor = ROTATION_VAL;
            }
            data.pFrame = pFrame;
        } // IVF
        else if (name.starts_with("movsteer")) {
            float maxAngle = 1.0f;
            if (name[8] == '_') {
                if (isdigit(name[9])) {
                    maxAngle = std::stof(&name[9]);
                }
            }
            data.factor = ROTATION_VAL * maxAngle;
            data.pFrame = pFrame;
        } else if (name.starts_with("steering_dummy")) {
            data.factor = ROTATION_VAL;
            data.pFrame = pFrame;
        } });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh); 
        if (!data.pFrame) {
            return;
        }

        float angle = pVeh->m_fSteerAngle * (1.666666f);
        MatrixUtil::ResetRotation(&data.pFrame->modelling);
        MatrixUtil::SetRotationY(&data.pFrame->modelling, angle*data.factor); 
        pVeh->UpdateRwFrame(); });
}