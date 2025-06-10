#include "pch.h"
#include "handlebar.h"
#include "core/materials.h"
#define TARGET_NODE "handlebars"
#define SOURCE_NODE "forks_front"

void HandleBar::Initialize()
{
    ModelMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, std::string name, bool parent)
                               { 
        auto &data = xData.Get(pVeh); 

        if (name == "forks_front") {
            data.m_pOrigin = pFrame;
        } else  if (name == "handlebars") {
            data.m_pTarget = pFrame;
        } });

    ModelMgr::RegisterRender([](CVehicle *pVeh)
                                {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh); 
        if (!data.m_pOrigin || !data.m_pTarget) {
            return;
        }

        float rot = Util::GetMatrixRotationZ(&data.m_pOrigin->modelling);
        Util::ResetMatrixRotations(&data.m_pTarget->modelling);
        Util::SetMatrixRotationZ(&data.m_pTarget->modelling, rot); 
        pVeh->UpdateRwFrame(); });
}