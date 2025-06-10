#include "pch.h"
#include "chain.h"
#include "core/materials.h"

const float minSpeed = 0.3f;
const float maxSpeed = 10.0f;
const float maxInterval = 200.0f;
const float minInterval = 20.0f;

void ChainFeature::Initialize()
{
    ModelMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame, std::string name, bool flag)
                               {
        if (name.starts_with("x_chain") || name.starts_with("fc_chain")) {
            VehData &data = vehData.Get(pVeh);
            data.m_pRootFrame = pFrame;
            Util::StoreChilds(pFrame, data.m_FrameList);
        } });

    ModelMgr::RegisterRender([](CVehicle *pVeh)
                                {
    if (!pVeh || !pVeh->GetIsOnScreen())
    {
        return;
    }
    VehData &data = vehData.Get(pVeh);
    if (data.m_FrameList.empty())
    {
        return;
    } 

    const short maxIndex = static_cast<short>(data.m_FrameList.size() - 1);
    const float speed = Util::GetVehicleSpeedRealistic(pVeh);
    const float absSpeed = std::fabs(speed);

    float interval = maxInterval;
    if (absSpeed > minSpeed)
    {
        float t = std::clamp((absSpeed - minSpeed) / (maxSpeed - minSpeed), 0.0f, 1.0f);
        interval = std::lerp(maxInterval, minInterval, t);
    }

    size_t curTime = CTimer::m_snTimeInMilliseconds;
    static size_t lastUpdateTime = 0;

    if (curTime - lastUpdateTime >= interval)
    {
        if (pVeh->m_nVehicleSubClass == VEHICLE_BMX)
        {
            if (pVeh->m_fGasPedal && speed > 0.0f) {
                data.m_nCurChain = (data.m_nCurChain == 0) ? maxIndex : data.m_nCurChain - 1;
            }
        }
        else
        {
            if (speed > minSpeed) {
                data.m_nCurChain = (data.m_nCurChain == 0) ? maxIndex : data.m_nCurChain - 1;
            }
            else if (speed < -minSpeed) {
                data.m_nCurChain = (data.m_nCurChain == maxIndex) ? 0 : data.m_nCurChain + 1;
            }
        }

        Util::HideAllChilds(data.m_pRootFrame);
        Util::ShowAllChilds(data.m_FrameList[data.m_nCurChain]);
        lastUpdateTime = curTime;
    } });
}