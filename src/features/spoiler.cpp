#include "pch.h"
#include "spoiler.h"
#include "utils/datamgr.h"
#include "utils/modelinfomgr.h"

void Spoiler::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        if (!nodeName.starts_with("movspoiler")) {
            return;
        }
        
        SpoilerVehData &data = m_VehData.Get(pVeh);
        SpoilerData spoilerData;
        auto first = nodeName.find('_');
        auto second = (first != std::string_view::npos) ? nodeName.find('_', first + 1) : std::string_view::npos;
        if (first != std::string_view::npos && second != std::string_view::npos && second > first + 1) {
            try {
                spoilerData.m_fRotation = std::stof(std::string(nodeName.substr(first + 1, second - first - 1)));
            } catch (...) {
                spoilerData.m_fRotation = 3.0f;
            }
        }
        else {
            spoilerData.m_fRotation = 3.0f;
        }

        auto last = nodeName.rfind('_');
        if (last != std::string_view::npos && last + 1 < nodeName.size()) {
            try {
                spoilerData.m_nTime = std::stof(std::string(nodeName.substr(last + 1)));
            } catch (...) {
                spoilerData.m_nTime = 3000.0f;
            }
        }
        else {
            spoilerData.m_nTime = 3000.0f;
        }

        spoilerData.m_pFrame = pFrame;

        auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
        std::string name(nodeName);
        if (jsonData.contains("spoilers") && jsonData["spoilers"].contains(name))
        {
            spoilerData.m_fRotation = jsonData["spoilers"][name].value("rotation", 30.0f);
            spoilerData.m_nTime = jsonData["spoilers"][name].value("time", 3000.0f);
            spoilerData.m_nTriggerSpeed = jsonData["spoilers"][name].value("triggerspeed", 20.0f);
        }
        else
        {
            spoilerData.m_nTriggerSpeed = 20.0f;
        }
        data.m_Spoilers.push_back(spoilerData);
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }

        SpoilerVehData &data = m_VehData.Get(pVeh); 
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

           MatrixUtil::ResetRotation(&e.m_pFrame->modelling);
           MatrixUtil::SetRotationXAbsolute(&e.m_pFrame->modelling, e.m_fCurrentRotation);
           RwMatrixUpdate(&e.m_pFrame->modelling);
        } });
}