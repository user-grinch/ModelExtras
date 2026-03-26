#include "pch.h"
#include "rollbackbed.h"
#include "modelinfomgr.h"
#include "datamgr.h"
#include "audiomgr.h"

using namespace plugin;

bool RollbackBed::UpdateRotation(CVehicle *pVeh, RwFrame *pFrame, float targetRot, float &curRot, float speed)
{
    VehData &data = xData.Get(pVeh);
    if (data.bInit && pFrame)
    {
        // TODO FIX
        // MatrixUtil::SetRotationX(&pFrame->modelling, curRot);
        float target = data.bExpanded ? targetRot : 0.0f;
        float delta = target - curRot;
        float step = CTimer::ms_fTimeStep * std::abs(targetRot) / 360.0f * speed;

        if (std::abs(delta) > step)
        {
            curRot += step * (delta > 0.0f ? 1.0f : -1.0f);
        }
        else
        {
            curRot = target;
            return true;
        }
    }
    return false;
}

bool RollbackBed::UpdateMove(HydraulicPiston &piston, float moveSpeed, bool bExpanded)
{
    if (piston.pFrame)
    {
        // 1. Determine destination based on state
        float target = bExpanded ? piston.fTargetMove : 0.0f;

        float delta = target - piston.fCurMove;
        
        // 2. Calculate Step (Velocity)
        // REMOVED: "piston.fTargetMove" from the formula.
        // The speed is now constant regardless of distance.
        // We divide by 100.0f as a base scaling factor so a speed of "1.0" isn't instant.
        float step = CTimer::ms_fTimeStep * (moveSpeed / 100.0f);

        // 3. Apply movement
        if (std::abs(delta) > step)
        {
            piston.fCurMove += step * (delta > 0.0f ? 1.0f : -1.0f);
        }
        else
        {
            piston.fCurMove = target;
        }

        // 4. Update Frame
        float frameDelta = piston.fCurMove - piston.fLastMove;
        
        piston.pFrame->modelling.pos.z += frameDelta;
        
        piston.fLastMove = piston.fCurMove;
        
        RwMatrixUpdate(&piston.pFrame->modelling);

        return piston.fCurMove == target;
    }
    return false;
}

void RollbackBed::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
    {
        std::string name = GetFrameNodeName(pFrame);
        if (!name.starts_with("x_rb"))
        {
            return;
        }
        
        VehData &data = xData.Get(pVeh);

        if (name == "x_rb_bed")
        {
            data.pBedFrame = pFrame;
        }

        if (name == "x_rb_hydraulics")
        {
            data.pHydralicsShellFrame = pFrame;
        }

        if (name.starts_with("x_rb_hydraulic_"))
        {
            // Initialize new piston with defaults
            HydraulicPiston newPiston;
            newPiston.pFrame = pFrame;
            newPiston.fTargetMove = 2.0f; 
            newPiston.fCurMove = 0.0f;
            newPiston.fLastMove = 0.0f;
            
            data.m_Pistons.push_back(newPiston);
        }

        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        if (jsonData.contains("rollback_bed"))
        {
            auto &jdata = jsonData["rollback_bed"];

            if (jdata.contains("hydraulics"))
            {
                auto &hydralics = jdata["hydraulics"];
                data.fHyTargetRot = hydralics.value("target_rot", data.fHyTargetRot);
                data.fHyRotSpeed = hydralics.value("rot_speed", data.fHyRotSpeed);
                
                // Load global config values
                float globalTargetMove = hydralics.value("target_move", 2.0f);
                data.fGlobalMoveSpeed = hydralics.value("move_speed", data.fGlobalMoveSpeed);

                // Apply the JSON target move to all pistons
                // (Unless you add specific logic here to parse an array of targets)
                for(auto& piston : data.m_Pistons) {
                    piston.fTargetMove = globalTargetMove;
                }
            }

            if (jdata.contains("bed"))
            {
                auto &bed = jdata["bed"];
                data.fBedRotSpeed = bed.value("rot_speed", data.fBedRotSpeed);
                data.fBedTargetRot = bed.value("target_rot", data.fBedTargetRot);
            }
        }

        data.bInit = true; 
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh);
        if (!data.bInit)
        {
            return;
        }

        UpdateRotation(pVeh, data.pBedFrame, data.fBedTargetRot, data.fBedCurRot, data.fBedRotSpeed);
        UpdateRotation(pVeh, data.pHydralicsShellFrame, data.fHyTargetRot, data.fHyCurRot, data.fHyRotSpeed);

        // Iterate over specific pistons
        for (auto& piston : data.m_Pistons)
        {
            UpdateMove(piston, data.fGlobalMoveSpeed, data.bExpanded);
        } 
    });

    Events::processScriptsEvent += []()
    {
        size_t now = CTimer::m_snTimeInMilliseconds;
        static size_t prev = 0;
        static uint32_t toggleKey = gConfig.ReadInteger("KEYS", "RollbackBedToggleKey", VK_B);

        if (KeyPressed(toggleKey) && now - prev > 500.0f)
        {
            CVehicle *pVeh = FindPlayerVehicle();
            if (pVeh)
            {
                VehData &data = xData.Get(pVeh);

                if (data.bInit)
                {
                    data.bExpanded = !data.bExpanded;
                    prev = now;
                    AudioMgr::PlaySwitchSound(pVeh);
                }
            }
        }
    };
}