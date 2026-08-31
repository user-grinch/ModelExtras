#include "pch.h"
#include "gauge.h"
#include "utils/datamgr.h"
#include <CBike.h>
#include "utils/modelinfomgr.h"

using namespace plugin;

static inline float ClampRotation(float value, float maxRot)
{
    float limit = std::abs(maxRot);
    return Clamp(value, -limit, limit);
}

void GearIndicator::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        if (nodeName.starts_with("x_gearmeter") || nodeName.starts_with("fc_gm")) {
            VehGearData &data = m_VehData.Get(pVeh);

            GearIndicatorData indData;
            indData.pRoot = pFrame;
            FrameUtil::StoreChilds(pFrame, indData.vecFrameList);
            data.vecIndicatorData.push_back(std::move(indData));
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::AnimatedGearMeter)) return;
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehGearData &data = m_VehData.Get(pVeh);

        for (auto&e : data.vecIndicatorData) {
            if (!e.vecFrameList.empty() &&  pVeh->m_nCurrentGear != e.iCurrent) {
                FrameUtil::HideAllChilds(e.pRoot);
                if (e.vecFrameList.size() > static_cast<size_t>(e.iCurrent))
                {
                    FrameUtil::ShowAllAtomics(e.vecFrameList[e.iCurrent]);
                }
                e.iCurrent = pVeh->m_nCurrentGear;
            }
        }
    });
}

void MileageIndicator::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName) {
        if (nodeName.starts_with("x_odometer") || nodeName.starts_with("fc_om")) {
            VehMileageData &data = m_VehData.Get(pVeh);
            std::string name(nodeName);
            auto& indicator = data.vecIndicatorData[name];

            FrameUtil::StoreChilds(pFrame, indicator.vecFrameList);

            indicator.dCurrentDistance = static_cast<double>(rand() % 999999);
            indicator.pFrame = pFrame;

            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData.contains("gauges") && jsonData["gauges"].contains(name)) {
                indicator.fMul = jsonData["gauges"][name].value("kph", true) ? 160.9f : 1.0f;
            }
            data.bInitialized = true;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh) {
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::AnimatedOdoMeter)) return;
    if (!pVeh || !pVeh->GetIsOnScreen()) return;

    VehMileageData &data = m_VehData.Get(pVeh);
    if (!data.bInitialized) return;

    for (auto& [name, indicator] : data.vecIndicatorData) {
        if (indicator.vecFrameList.size() < 6) continue;

        float curWheelRot = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
            ? static_cast<CBike *>(pVeh)->m_aWheelPitchAngles[1]
            : static_cast<CAutomobile *>(pVeh)->m_fWheelRotation[3];

        float diff = curWheelRot - indicator.fLastWheelRot;
        if (abs(diff) > 5.0f) diff = 0.0f;

        CVehicleModelInfo *pModelInfo = static_cast<CVehicleModelInfo *>(CModelInfo::GetModelInfo(pVeh->m_nModelIndex));
        float wheelRadius = (pModelInfo && pModelInfo->m_fWheelSizeRear > 0.0f) ? pModelInfo->m_fWheelSizeRear : 0.35f;
        float wheelDivisor = (wheelRadius * 8.17f) * indicator.fMul;
        indicator.dCurrentDistance += (abs(diff) / (wheelDivisor > 0.0f ? wheelDivisor : 2.86f));
        indicator.fLastWheelRot = curWheelRot;

        int displayVal = static_cast<int>(indicator.dCurrentDistance) % 1000000;

        int divisor = 100000;
        for (int i = 0; i < 6; i++) {
            int currentDigit = (displayVal / divisor) % 10;
            divisor /= 10;

            if (indicator.lastDigits[i] != currentDigit) {
                if (indicator.lastDigits[i] != -1) {
                    int steps = currentDigit - indicator.lastDigits[i];
                    if (steps < 0) steps += 10;

                    float angleToRotate = static_cast<float>(steps) * 36.0f;
                    FrameUtil::SetRotationX(indicator.vecFrameList[i], angleToRotate);
                } else {
                    FrameUtil::SetRotationX(indicator.vecFrameList[i], currentDigit * 36.0f);
                }

                indicator.lastDigits[i] = currentDigit;
            }
        }
    }
});
}

void RPMGauge::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        if (nodeName.starts_with("x_rpm") || nodeName.starts_with("fc_rpm") || nodeName.starts_with("tahook")) {
            VehRPMData &data = m_VehData.Get(pVeh);
            std::string name(nodeName);
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData.contains("gauges") && jsonData["gauges"].contains(name))
            {
                if (jsonData["gauges"][name].contains("maxrpm"))
                {
                    data.vecGaugeData[name].iMaxRPM = jsonData["gauges"][name].value("maxrpm", data.vecGaugeData[name].iMaxRPM);
                }
                if (jsonData["gauges"][name].contains("maxrotation"))
                {
                    data.vecGaugeData[name].fMaxRotation = jsonData["gauges"][name].value("maxrotation", data.vecGaugeData[name].fMaxRotation);
                }
            }
            data.vecGaugeData[name].pFrame = pFrame;
            data.bInitialized = true;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::AnimatedRpmMeter)) return;
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehRPMData &data = m_VehData.Get(pVeh);
        if (data.bInitialized) {
            float delta = CTimer::ms_fTimeStep;
            float speed = Util::GetVehicleSpeedRealistic(pVeh);

            for (auto& e : data.vecGaugeData) {
                float rpm = 0.0f;

                if (pVeh->m_nCurrentGear != 0) {
                    if (pVeh->m_pHandlingData && pVeh->m_nCurrentGear > 0 && pVeh->m_nCurrentGear <= pVeh->m_pHandlingData->m_transmissionData.m_nNumberOfGears) {
                        float maxGearVel = pVeh->m_pHandlingData->m_transmissionData.m_aGears[pVeh->m_nCurrentGear].m_fMaxVelocity;
                        if (maxGearVel > 0.0f) {
                            rpm = std::clamp((speed / (maxGearVel * 160.9f)), 0.1f, 1.0f) * e.second.iMaxRPM;
                        } else {
                            rpm = (speed / abs((float)pVeh->m_nCurrentGear)) * 100.0f;
                        }
                    } else {
                        rpm = (speed / abs((float)pVeh->m_nCurrentGear)) * 100.0f;
                    }
                }

                if (pVeh->bEngineOn) {
                  rpm = std::max(rpm, 0.1f * e.second.iMaxRPM);
                }

                float targetRotation = (rpm / (float)e.second.iMaxRPM) * e.second.fMaxRotation;
                targetRotation = ClampRotation(targetRotation, e.second.fMaxRotation);

                float change = (targetRotation - e.second.fCurRotation) * 0.25f * delta;
                FrameUtil::SetRotationY(e.second.pFrame, change);
                e.second.fCurRotation += change;
            }
        }
    });
}

void SpeedGauge::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        if (nodeName.starts_with("x_sm") || nodeName.starts_with("fc_sm") || nodeName.starts_with("speedook")) {
            VehSpeedData &data = m_VehData.Get(pVeh);
            std::string name(nodeName);
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData.contains("gauges") && jsonData["gauges"].contains(name))
            {
                if (jsonData["gauges"][name].contains("kph"))
                {
                    data.vecGaugeData[name].fMul = jsonData["gauges"][name]["kph"].get<bool>() ? 160.9f : 1;
                }
                if (jsonData["gauges"][name].contains("maxspeed"))
                {
                    data.vecGaugeData[name].iMaxSpeed = jsonData["gauges"][name].value("maxspeed", data.vecGaugeData[name].iMaxSpeed);
                }
                if (jsonData["gauges"][name].contains("maxrotation"))
                {
                    data.vecGaugeData[name].fMaxRotation = jsonData["gauges"][name].value("maxrotation", data.vecGaugeData[name].fMaxRotation);
                }
            }
            data.vecGaugeData[name].pFrame = pFrame;
            data.bInitialized = true;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::AnimatedSpeedMeter)) return;
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehSpeedData &data = m_VehData.Get(pVeh);
        if (data.bInitialized) {
            float speed = Util::GetVehicleSpeedRealistic(pVeh);
            float delta = CTimer::ms_fTimeStep;

            for (auto& e : data.vecGaugeData) {
                float targetRotation = (speed / (float)e.second.iMaxSpeed) * e.second.fMaxRotation;
                // Stop reverse gear from moving to opposite direction
                if (pVeh->m_nCurrentGear == 0) {
                    targetRotation = -targetRotation;
                }
                targetRotation = ClampRotation(targetRotation, e.second.fMaxRotation);
                float change = (targetRotation - e.second.fCurRotation) * 0.5f * delta;
                FrameUtil::SetRotationY(e.second.pFrame, change);
                e.second.fCurRotation += change;
            }
        }
    });
}

void TurboGauge::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        if (nodeName.starts_with("x_tm")) {
            VehTurboData &data = m_VehData.Get(pVeh);
            std::string name(nodeName);
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData.contains("gauges") && jsonData["gauges"].contains(name))
            {
                if (jsonData["gauges"][name].contains("maxturbo"))
                {
                    data.vecGaugeData[name].iMaxTurbo = jsonData["gauges"][name].value("maxturbo", data.vecGaugeData[name].iMaxTurbo);
                }
                if (jsonData["gauges"][name].contains("maxrotation"))
                {
                    data.vecGaugeData[name].fMaxRotation = jsonData["gauges"][name].value("maxrotation", data.vecGaugeData[name].fMaxRotation);
                }
            }
            data.vecGaugeData[name].pFrame = pFrame;
            data.bInitialized = true;
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::AnimatedTurboMeter)) return;
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehTurboData &data = m_VehData.Get(pVeh);
        if (data.bInitialized) {
            float speed = Util::GetVehicleSpeedRealistic(pVeh);
            float delta = CTimer::ms_fTimeStep;

            for (auto& e : data.vecGaugeData) {
                float turbo = speed - e.second.fPrevTurbo;

                if (pVeh->m_nCurrentGear != 0)
                {
                    turbo += (turbo >= 0) ? 10.0f : -10.0f;
                }

                float targetRotation = (e.second.fMaxRotation / (float)e.second.iMaxTurbo) * turbo * delta;
                // Stop reverse gear from moving to opposite direction
                if (pVeh->m_nCurrentGear == 0) {
                    targetRotation = -targetRotation;
                }
                targetRotation = ClampRotation(targetRotation, e.second.fMaxRotation);
                float change = (targetRotation - e.second.fCurRotation) * 0.25f * delta;
                FrameUtil::SetRotationY(e.second.pFrame, change);
                e.second.fCurRotation += change;
                e.second.fPrevTurbo = speed;
            }
        }
    });
}

void FixedGauge::Init()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, const std::string_view nodeName)
    {
        if (nodeName.starts_with("x_gauge_fixed") || nodeName == "x_gasmeter" || nodeName == "x_gm" || nodeName == "petrolok") {
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);

            float minAngle = 20.0f;
            float maxAngle = 70.0f;
            std::string name(nodeName);
            if (jsonData.contains("gauges") && jsonData["gauges"].contains(name)) {
                minAngle = jsonData["gauges"][name].value("minangle", minAngle);
                maxAngle = jsonData["gauges"][name].value("maxangle", maxAngle);
            }
            FrameUtil::SetRotationY(pFrame, RandomNumberInRange(minAngle, maxAngle));
        }
    });
}