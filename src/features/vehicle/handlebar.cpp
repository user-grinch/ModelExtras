#include "pch.h"
#include "handlebar.h"
#define TARGET_NODE "handlebars"
#define SOURCE_NODE "forks_front"

void HandleBar::AddSource(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);
    VehData& data = xData.Get(pVeh);
    data.m_pSource = frame;
}

void HandleBar::Process(void* ptr, RwFrame* frame, eModelEntityType type) {
    CVehicle* pVeh = static_cast<CVehicle*>(ptr);

    VehData& data = xData.Get(pVeh);
    if (data.m_pSource) {
        float rot = Util::GetMatrixRotationZ(&data.m_pSource->modelling);
        Util::SetMatrixRotationZ(&frame->modelling, rot);
    }
}