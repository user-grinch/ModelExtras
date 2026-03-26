#include "pch.h"
#include "steerwheel.h"
#include "modelinfomgr.h"

void SteerWheel::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
		if (gbVehIKInstalled) {
            return;
		}
        auto& data = xData.Get(pVeh);

        // VehFuncs
        std::string name = GetFrameNodeName(pFrame);
        if (Hash::StartsWith(name, "f_steer")) {
            if (name.length() >= 7 && isdigit(name[7]))
            {
                data.factor = (float)std::stoi(&name[7]) / 2;
            }
            data.pFrame = pFrame;
        } else if (Hash::StartsWith(name, "movsteer") || Hash::StartsWith(name, "steering_dummy") || Hash::StartsWith(name, "ik_steer")) {
            data.pFrame = pFrame;
        } 
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
        if (gbVehIKInstalled || !pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh); 
        if (!data.pFrame) {
            return;
        }

        float angle = Util::RadToDeg(pVeh->m_fSteerAngle);
        if (std::abs(angle) > 1.0f) {
            MatrixUtil::SetRotationYAbsolute(&data.pFrame->modelling, (angle - data.prevAngle) * data.factor); 
            data.prevAngle = angle;
        }
    });
}