#include "pch.h"
#include "spoiler.h"
#include "datamgr.h"
#include "avs/materials.h"

void Spoiler::Initialize()
{
    VehicleMaterials::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, std::string name, bool parent)
                                    {
        if (!name.starts_with("movspoiler")) {
            return;
        }
        
        VehData &data = xData.Get(pVeh);
        SpoilerData spoilerData;
        spoilerData.m_fRotation = std::stof(Util::GetRegexVal(name, "_(.*?)_", "30.0"));
        spoilerData.m_nTime = std::stof(Util::GetRegexVal(name, "_([^_]*)$", "3000"));
        spoilerData.m_pFrame = pFrame;

        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        if (jsonData["spoilers"].contains(name))
        {
            spoilerData.m_fRotation = jsonData["spoilers"][name].value("rotation", 30.0f);
            spoilerData.m_nTime = jsonData["spoilers"][name].value("time", 3000);
            spoilerData.m_nTriggerSpeed = jsonData["spoilers"][name].value("triggerspeed", 20);
        }
        else
        {
            spoilerData.m_nTriggerSpeed = 20;
        }
        spoilerData.m_fCurrentRotation = 0.0f; 
        data.m_Spoilers.push_back(spoilerData); });

    VehicleMaterials::RegisterRender([](CVehicle *pVeh)
                                     {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        VehData &data = xData.Get(pVeh); 
        if (data.m_Spoilers.size() == 0) {
            return;
        }

        for (auto& e: data.m_Spoilers) {
            bool isEnabled = Util::GetVehicleSpeed(pVeh) > e.m_nTriggerSpeed;

            float targetAngle = isEnabled ? -e.m_fRotation : 0.0f;
            float totalTime = std::max(1.0f, static_cast<float>(e.m_nTime));

            float transitionSpeed = (isEnabled ? 10.0f : 15.0f) / totalTime;

            // Smoothing
            float t = 1.0f - std::exp(-transitionSpeed * CTimer::ms_fTimeStep);

            e.m_fCurrentRotation =
            e.m_fCurrentRotation * (1.0f - t) + targetAngle * t;

            Util::ResetMatrixRotations(&e.m_pFrame->modelling);
            Util::SetMatrixRotationX(&e.m_pFrame->modelling, e.m_fCurrentRotation);
            RwMatrixUpdate(&e.m_pFrame->modelling);
        } });
}