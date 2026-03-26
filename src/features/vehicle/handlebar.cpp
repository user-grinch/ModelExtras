#include "pch.h"
#include "handlebar.h"
#include "modelinfomgr.h"

void HandleBar::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
    { 
        if (gbVehIKInstalled) {
            return;
		}
        auto &data = xData.Get(pVeh); 
        std::string name = GetFrameNodeName(pFrame);
        if (name == "forks_front") {
            data.m_pOrigin = pFrame;
        } else  if (name == "handlebars") {
            data.m_pTarget = pFrame;
        } 
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (gbVehIKInstalled || !pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh); 
        if (!data.m_pOrigin || !data.m_pTarget) {
            return;
        }

        float rot = MatrixUtil::GetRotationZ(&data.m_pOrigin->modelling);
        MatrixUtil::SetRotationZAbsolute(&data.m_pTarget->modelling, rot - data.prevAngle); 
        data.prevAngle = rot;
    });
}