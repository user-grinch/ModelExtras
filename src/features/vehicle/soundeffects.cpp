#include "pch.h"
#include "soundeffects.h"

void SoundEffects::Initialize()
{
    plugin::Events::vehicleRenderEvent += [](CVehicle *pVeh)
    {
        if (pVeh == FindPlayerVehicle(0, false))
        {
            static std::string path = MOD_DATA_PATH("audio/effects/reverse.wav");
            auto &data = vehData.Get(pVeh);
            float speed = pVeh->m_vecMoveSpeed.Magnitude2D() * 50.0f;
            if (pVeh->m_nCurrentGear == 0 && pVeh->m_nVehicleFlags.bEngineOn && !pVeh->m_nVehicleFlags.bEngineBroken && speed >= 3.0f)
            {
                if (m_hReverse == NULL)
                {
                    m_hReverse = AudioMgr::Load(&path);
                }
                AudioMgr::Play(m_hReverse, pVeh);
            }

            if (data.m_bEngineState != pVeh->m_nVehicleFlags.bEngineOn)
            {
                static std::string path = MOD_DATA_PATH("audio/effects/engine-start.wav");
                if (pVeh->m_nVehicleFlags.bEngineOn)
                {
                    AudioMgr::LoadAndPlay(&path, pVeh);
                }
                data.m_bEngineState = pVeh->m_nVehicleFlags.bEngineOn;
            }
        }
    };
}