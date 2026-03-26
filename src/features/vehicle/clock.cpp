#include "pch.h"
#include "clock.h"
#include <CClock.h>
#include "datamgr.h"
#include "modelinfomgr.h"

void DigitalClockFeature::Initialize()
{
    LOG_VERBOSE("Init {}", __FUNCTION__);
    ModelInfoMgr::RegisterDummy([](CVehicle *pVeh, RwFrame *pFrame)
    {
        std::string name = GetFrameNodeName(pFrame);
        if (name.starts_with("x_dclock")) {
            VehData &data = vehData.Get(pVeh);
            data.m_pRootFrame = pFrame;

            auto &jsonData = DataMgr::Get(pVeh->m_nModelIndex);
            if (jsonData["clocks"].contains(name))
            {
                data.m_b12HourFormat = jsonData["clocks"][name].value("12hformat", false);
            }

            RwFrame *pCur = pFrame->child;

            int flag = 0;
            while (pCur)
            {
                name = GetFrameNodeName(pCur);
                if (data.m_pDigitsRoot == nullptr && name == "digits")
                {
                    data.m_pDigitsRoot = pCur;
                    flag++;
                }

                for (int i = 0; i < 4; i++)
                {
                    if (data.m_pDigitPos[i] == nullptr && name == std::format("digit{}", i+1))
                    {
                        data.m_pDigitPos[i] = pCur;
                        flag++;
                    }
                }
                pCur = pCur->next;
            }

            if (flag == 5 && data.m_pDigitsRoot->child)
            {
                RpAtomic * tempAtomic;
                RpAtomic * newAtomic;

                RwFrame *pCur = data.m_pDigitsRoot->child;

                for (int i = 0; i < 10; i++)
                {
                    if (!pCur) break;

                    tempAtomic = (RpAtomic *)GetFirstObject(pCur);
                    if (tempAtomic)
                    {
                        for (int j = 3; j >= 0; j--)
                        {
                            newAtomic = RpAtomicClone(tempAtomic);
                            RpAtomicSetFrame(newAtomic, data.m_pDigitPos[j]);
                            RpClumpAddAtomic(pVeh->m_pRwClump, newAtomic);
                        }
                    }
                    pCur = pCur->next;
                }
                FrameUtil::DestroyNodeHierarchyRecursive(data.m_pDigitsRoot);
            }
        }
    });

    ModelInfoMgr::RegisterRender([](CVehicle *pVeh)
    {
        if (!pVeh || !pVeh->GetIsOnScreen())
        {
            return;
        }
        VehData &data = vehData.Get(pVeh);

        if (!data.m_pDigitPos[0] || !data.m_pDigitPos[1] || !data.m_pDigitPos[2] || !data.m_pDigitPos[3])
            return;

        int hour = CClock::ms_nGameClockHours;
        int min = CClock::ms_nGameClockMinutes;

        if (data.m_b12HourFormat)
        {
            hour = hour % 12;
            if (hour == 0) hour = 12;
        }

        int h1 = hour / 10;
        int h2 = hour % 10;
        int m1 = min / 10;
        int m2 = min % 10;

        FrameUtil::HideAllAtomicsExcept(data.m_pDigitPos[0], abs(h1 - 9));
        FrameUtil::HideAllAtomicsExcept(data.m_pDigitPos[1], abs(h2 - 9));
        FrameUtil::HideAllAtomicsExcept(data.m_pDigitPos[2], abs(m1 - 9));
        FrameUtil::HideAllAtomicsExcept(data.m_pDigitPos[3], abs(m2 - 9));
    });
}