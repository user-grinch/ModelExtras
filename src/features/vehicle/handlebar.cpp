#include "pch.h"
#include "handlebar.h"
#define TARGET_NODE "handlebars"
#define SOURCE_NODE "forks_front"

HandleBarFeature HandleBar;

void HandleBarFeature::Process(RwFrame* frame, CVehicle* pVeh) {
    std::string name = GetFrameNodeName(frame);
    bool src = NODE_FOUND(name, SOURCE_NODE);
    bool target = NODE_FOUND(name, TARGET_NODE);
    if (src || target) {
        VehData &data = xData.Get(pVeh);
        if (data.m_pHandleBar && data.m_pForkFront)  {
            float rot = Util::GetMatrixRotationZ(&data.m_pForkFront->modelling);
            Util::SetMatrixRotationZ(&data.m_pHandleBar->modelling, rot);
        } else {
            if (src) {
                data.m_pForkFront =  frame;
            }

            if (target) {
                data.m_pHandleBar = frame;
            }
        }
    }
}