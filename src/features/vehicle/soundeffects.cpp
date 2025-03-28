#include "pch.h"
#include "soundeffects.h"
#include "lights.h"

std::vector<int> ValidForReverseSound = {};

void SoundEffects::Initialize()
{
    plugin::Events::initGameEvent += []()
    {
        std::string line = gConfig.ReadString("TABLE", "SoundEffects_GlobalReverseSoundModels", "");
        std::stringstream ss(line);
        while (ss.good())
        {
            std::string model;
            getline(ss, model, ',');
            ValidForReverseSound.push_back(std::stoi(model));
        }
    };

    plugin::Events::vehicleRenderEvent += [](CVehicle *pVeh)
    {
        static bool bReverseSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalReverseSound", false);
        static bool bEngineSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalEngineSound", false);
        static bool bIndicatorSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalIndicatorSound", false);

        if (pVeh == FindPlayerVehicle(0, false))
        {
            auto &data = vehData.Get(pVeh);
            if (bEngineSounds)
            {
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

            if (bIndicatorSounds)
            {
                bool state = Lights::IsIndicatorOn(pVeh);
                if (state != data.m_bIndicatorState)
                {
                    static std::string onpath = MOD_DATA_PATH("audio/effects/indicator_on.wav");
                    static std::string offpath = MOD_DATA_PATH("audio/effects/indicator_off.wav");
                    static StreamHandle hOn = NULL, hOff = NULL;
                    if (hOn == NULL)
                    {
                        hOn = AudioMgr::Load(&onpath);
                    }
                    if (hOff == NULL)
                    {
                        hOff = AudioMgr::Load(&offpath);
                    }
                    AudioMgr::Play(state ? hOn : hOff, pVeh);
                    data.m_bIndicatorState = state;
                }
            }

            if (bReverseSounds)
            {
                static std::string path = MOD_DATA_PATH("audio/effects/reverse.wav");
                float speed = pVeh->m_vecMoveSpeed.Magnitude2D() * 50.0f;
                bool valid = std::find(ValidForReverseSound.begin(), ValidForReverseSound.end(), pVeh->m_nModelIndex) != ValidForReverseSound.end();
                if (valid && pVeh->m_nCurrentGear == 0 && pVeh->m_nVehicleFlags.bEngineOn && !pVeh->m_nVehicleFlags.bEngineBroken && speed >= 3.0f)
                {
                    if (m_hReverse == NULL)
                    {
                        m_hReverse = AudioMgr::Load(&path);
                    }
                    AudioMgr::Play(m_hReverse, pVeh);
                }
            }
        }
    };
}