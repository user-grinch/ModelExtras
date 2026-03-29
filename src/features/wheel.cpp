#include "pch.h"
#include "wheel.h"
#include "utils/modelinfomgr.h"
#include <string_view>

void UpdateWheelRotation(CVehicle *pVeh, RwFrame *ori, RwFrame *tar)
{
    if (ori && tar)
    {
        // MatrixUtil::SetRotationZ(&tar->modelling, MatrixUtil::GetRotationZ(&ori->modelling));
        // MatrixUtil::SetRotationY(&tar->modelling, MatrixUtil::GetRotationY(&ori->modelling));
        MatrixUtil::SetRotationXAbsolute(&tar->modelling, MatrixUtil::GetRotationX(&ori->modelling) - MatrixUtil::GetRotationX(&tar->modelling));
        pVeh->UpdateRwFrame();
    }
}


static const std::unordered_map<std::string_view, eWheelPos> originalMap = {
    { "wheel_rf_dummy", eWheelPos::RightFront },
    { "wheel_rm_dummy", eWheelPos::RightMiddle },
    { "wheel_rr_dummy", eWheelPos::RightRear },
    { "wheel_rb_dummy", eWheelPos::RightRear },
    { "wheel_lf_dummy", eWheelPos::LeftFront },
    { "wheel_lm_dummy", eWheelPos::LeftMiddle },
    { "wheel_lr_dummy", eWheelPos::LeftRear },
    { "wheel_lb_dummy", eWheelPos::LeftRear },
};

static const std::unordered_map<std::string_view, eWheelPos> extraMap = {
    { "x_wheel_lf", eWheelPos::LeftFront },
    { "x_wheel_lm", eWheelPos::LeftMiddle },
    { "x_wheel_lr", eWheelPos::LeftRear },
    { "x_wheel_rf", eWheelPos::RightFront },
    { "x_wheel_rm", eWheelPos::RightMiddle },
    { "x_wheel_rr", eWheelPos::RightRear },
};

void ExtraWheel::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame, const std::string_view name) {
        ExtraWheelData& data = m_VehData.Get(pVeh);

        auto itOrig = originalMap.find(name);
        if (itOrig != originalMap.end()) {
            data.pOriginals[(int)itOrig->second].push_back(pFrame);
            return;
        }

        if (!name.starts_with("x_wheel")) {
            return;
        }

        std::string_view prefix = name.substr(0, 10);
        auto itExtra = extraMap.find(prefix);
        if (itExtra != extraMap.end()) {
            data.pExtras[(int)itExtra->second].push_back(pFrame);
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        ExtraWheelData &data = m_VehData.Get(pVeh);

        for (int i = 0; i < static_cast<int>(eWheelPos::COUNT); i++)
        {
            for (int j = 0; j < data.pExtras[i].size(); j++) {
                UpdateWheelRotation(pVeh, data.pOriginals[i][j], data.pExtras[i][j]);
            }
        } });
}