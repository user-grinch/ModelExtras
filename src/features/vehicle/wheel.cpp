#include "pch.h"
#include "wheel.h"
#include "modelinfomgr.h"

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

void ExtraWheel::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle* pVeh, RwFrame* pFrame) {
        VehData& data = xData.Get(pVeh);
        std::string_view name = GetFrameNodeName(pFrame);
        Hash::Value nameHash = Hash::Get(name);

        switch (nameHash) {
            case "wheel_rf_dummy"_h:
                data.pOriginals[static_cast<int>(eWheelPos::RightFront)].push_back(pFrame);
                break;
            case "wheel_rm_dummy"_h:
                data.pOriginals[static_cast<int>(eWheelPos::RightMiddle)].push_back(pFrame);
                break;
            case "wheel_rr_dummy"_h:
            case "wheel_rb_dummy"_h:
                data.pOriginals[static_cast<int>(eWheelPos::RightRear)].push_back(pFrame);
                break;
            case "wheel_lf_dummy"_h:
                data.pOriginals[static_cast<int>(eWheelPos::LeftFront)].push_back(pFrame);
                break;
            case "wheel_lm_dummy"_h:
                data.pOriginals[static_cast<int>(eWheelPos::LeftMiddle)].push_back(pFrame);
                break;
            case "wheel_lr_dummy"_h:
            case "wheel_lb_dummy"_h:
                data.pOriginals[static_cast<int>(eWheelPos::LeftRear)].push_back(pFrame);
                break;
        }

        if (!Hash::StartsWith(name, "x_wheel") || name.length() < 10) {
            return;
        }

        Hash::Value prefixHash = Hash::Get(name.substr(0, 10));
        switch (prefixHash) {
            case "x_wheel_lf"_h:
                data.pExtras[static_cast<int>(eWheelPos::LeftFront)].push_back(pFrame);
                break;
            case "x_wheel_lm"_h:
                data.pExtras[static_cast<int>(eWheelPos::LeftMiddle)].push_back(pFrame);
                break;
            case "x_wheel_lr"_h:
                data.pExtras[static_cast<int>(eWheelPos::LeftRear)].push_back(pFrame);
                break;
            case "x_wheel_rf"_h:
                data.pExtras[static_cast<int>(eWheelPos::RightFront)].push_back(pFrame);
                break;
            case "x_wheel_rm"_h:
                data.pExtras[static_cast<int>(eWheelPos::RightMiddle)].push_back(pFrame);
                break;
            case "x_wheel_rr"_h:
                data.pExtras[static_cast<int>(eWheelPos::RightRear)].push_back(pFrame);
                break;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh);

        for (int i = 0; i < data.pExtras.size(); i++)
        {
            for (int j = 0; j < data.pExtras[i].size(); j++) {
                UpdateWheelRotation(pVeh, data.pOriginals[i][j], data.pExtras[i][j]);
            }
        } });
}