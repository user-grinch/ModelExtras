#include "pch.h"
#include "handlebar.h"
#include "modelinfomgr.h"
#define TARGET_NODE "handlebars"
#define SOURCE_NODE "forks_front"

void HandleBar::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                               { 
        auto &data = xData.Get(pVeh); 
        std::string name = GetFrameNodeName(pFrame);
        if (name == "forks_front") {
            data.m_pOrigin = pFrame;
        } else  if (name == "handlebars") {
            data.m_pTarget = pFrame;
        } });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh); 
        if (!data.m_pOrigin || !data.m_pTarget) {
            return;
        }

        float rot = MatrixUtil::GetRotationZ(&data.m_pOrigin->modelling);
        MatrixUtil::ResetRotation(&data.m_pTarget->modelling);
        MatrixUtil::SetRotationZ(&data.m_pTarget->modelling, rot); 
        pVeh->UpdateRwFrame(); });
}