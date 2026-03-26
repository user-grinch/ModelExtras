#include "pch.h"
#include "gauge.h"
#include "datamgr.h"
#include <CBike.h>
#include "modelinfomgr.h"

using namespace plugin;

static inline float ClampRotation(float value, float maxRot)
{
    float limit = std::abs(maxRot);
    return Clamp(value, -limit, limit);
}

void GearIndicator::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_gearmeter") || name.starts_with("fc_gm")) {
            VehData &data = vehData.Get(pVeh);

            IndicatorData indData;
            indData.pRoot = pFrame;
            FrameUtil::StoreChilds(pFrame, indData.vecFrameList);
            data.vecIndicatorData.push_back(std::move(indData));
        } });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData &data = vehData.Get(pVeh);

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

void MileageIndicator::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame) {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_odometer") || name.starts_with("fc_om")) {
            VehData &data = vehData.Get(pVeh);
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
    if (!pVeh || !pVeh->GetIsOnScreen()) return;

    VehData &data = vehData.Get(pVeh);
    if (!data.bInitialized) return;

    for (auto& [name, indicator] : data.vecIndicatorData) {
        if (indicator.vecFrameList.size() < 6) continue;

        float curWheelRot = (pVeh->m_nVehicleSubClass == VEHICLE_BIKE)
            ? static_cast<CBike *>(pVeh)->m_afWheelRotationX[1]
            : static_cast<CAutomobile *>(pVeh)->m_fWheelRotation[3];

        float diff = curWheelRot - indicator.fLastWheelRot;
        if (abs(diff) > 5.0f) diff = 0.0f;

        indicator.dCurrentDistance += (abs(diff) / (2.86f * indicator.fMul));
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

void RPMGauge::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_rpm") || name.starts_with("fc_rpm") || name.starts_with("tahook")) {
            VehData &data = vehData.Get(pVeh);
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
        } });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData &data = vehData.Get(pVeh);
        if (data.bInitialized) {
            float delta = CTimer::ms_fTimeScale;
            float speed = Util::GetVehicleSpeedRealistic(pVeh);

            for (auto& e : data.vecGaugeData) {
                float rpm = 0.0f;

                if (pVeh->m_nCurrentGear != 0) {
                  rpm = (speed / abs((float)pVeh->m_nCurrentGear)) * 100.0f;
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
        } });
}

void SpeedGauge::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
    std::string name = GetFrameNodeName(pFrame);
    if (name.starts_with("x_sm") || name.starts_with("fc_sm") || name.starts_with("speedook")) {
        VehData &data = vehData.Get(pVeh);
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
    } });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData &data = vehData.Get(pVeh);
        if (data.bInitialized) {
            float speed = Util::GetVehicleSpeedRealistic(pVeh);
            float delta = CTimer::ms_fTimeScale;

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
        } });
}

void TurboGauge::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_tm")) {
            VehData &data = vehData.Get(pVeh);
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
        } });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
                                 {
        if (!pVeh || !pVeh->GetIsOnScreen()) return;

        VehData &data = vehData.Get(pVeh);
        if (data.bInitialized) {
            float speed = Util::GetVehicleSpeedRealistic(pVeh);
            float delta = CTimer::ms_fTimeScale;

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
        } });
}

void FixedGauge::Initialize()
{
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
                                {
        std::string name = GetFrameNodeName(pFrame);

        if (name.starts_with("x_gauge_fixed") || name == "x_gasmeter" || name == "x_gm" || name == "petrolok") {
            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);

            float minAngle = 20.0f;
            float maxAngle = 70.0f;
            if (jsonData.contains("gauges") && jsonData["gauges"][name].contains("angle")) {
                minAngle = jsonData["gauges"][name]["minangle"];
                maxAngle = jsonData["gauges"][name]["maxangle"];
            }
            FrameUtil::SetRotationY(pFrame, RandomNumberInRange(minAngle, maxAngle));
        } });
}