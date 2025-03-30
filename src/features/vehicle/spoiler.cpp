#include "pch.h"
#include "spoiler.h"
#include "datamgr.h"

void Spoiler::Initialize(void *ptr, RwFrame *pFrame, eModelEntityType type)
{
    CVehicle *pVeh = static_cast<CVehicle *>(ptr);
    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(pFrame);
    data.m_Spoilers[name].m_fRotation = std::stof(Util::GetRegexVal(name, "_(.*?)_", "30.0"));
    data.m_Spoilers[name].m_nTime = std::stof(Util::GetRegexVal(name, "_([^_]*)$", "3000"));

    auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
    if (jsonData["Spoilers"].contains(name))
    {
        data.m_Spoilers[name].m_fRotation = jsonData["Spoilers"][name].value("Rotation", 30.0f);
        data.m_Spoilers[name].m_nTime = jsonData["Spoilers"][name].value("Time", 3000);
        data.m_Spoilers[name].m_nTriggerSpeed = jsonData["Spoilers"][name].value("TriggerSpeed", 20);
    }
    else
    {
        data.m_Spoilers[name].m_nTriggerSpeed = 20;
    }
    data.m_Spoilers[name].m_fCurrentRotation = 0.0f;
}

void Spoiler::Process(void *ptr, RwFrame *pFrame, eModelEntityType type)
{
    CVehicle *pVeh = static_cast<CVehicle *>(ptr);
    if (!pVeh)
        return;

    VehData &data = vehData.Get(pVeh);
    std::string name = GetFrameNodeName(pFrame);

    if (!data.m_bInit)
    {
        Initialize(ptr, pFrame, type);
        data.m_bInit = true;
    }

    bool isEnabled = Util::GetVehicleSpeed(pVeh) > data.m_Spoilers[name].m_nTriggerSpeed;

    float targetAngle = isEnabled ? -data.m_Spoilers[name].m_fRotation : 0.0f;
    float totalTime = std::max(1.0f, static_cast<float>(data.m_Spoilers[name].m_nTime));

    // Make down anim faster
    float transitionSpeed = (isEnabled ? 5.0f : 10.0f) / totalTime;

    // Smoothing
    float t = 1.0f - std::exp(-transitionSpeed * CTimer::ms_fTimeStep);

    data.m_Spoilers[name].m_fCurrentRotation =
        data.m_Spoilers[name].m_fCurrentRotation * (1.0f - t) + targetAngle * t;

    Util::ResetMatrixRotations(&pFrame->modelling);
    Util::SetMatrixRotationX(&pFrame->modelling, data.m_Spoilers[name].m_fCurrentRotation);
    RwMatrixUpdate(&pFrame->modelling);
}
