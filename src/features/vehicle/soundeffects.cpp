#include "pch.h"
#include "soundeffects.h"
#include "lights.h"
#include "eVehicleClass.h"
#include "util.h"

std::vector<int> ValidForReverseSound;

#define ANIMGROUP_TRUCK 2
#define ANIMGROUP_BUS 15
#define ANIMGROUP_COACH 16

void SoundEffects::Initialize()
{
    plugin::Events::initGameEvent += []()
    {
        std::string line = gConfig.ReadString("TABLE", "SoundEffects_BigVehicleModels", "");
        Util::GetModelsFromIni(line, ValidForReverseSound);
    };

    plugin::Events::vehicleRenderEvent += [](CVehicle *pVeh)
    {
        static bool bReverseSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalReverseSound", false);
        static bool bEngineSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalEngineSound", false);
        static bool bIndicatorSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalIndicatorSound", false);
        static bool bAirbreakSounds = gConfig.ReadBoolean("VEHICLE_FEATURES", "SoundEffects_GlobalAirbreakSound", false);

        auto &data = vehData.Get(pVeh);
        float speed = Util::GetVehicleSpeed(pVeh);
        int model = pVeh->m_nModelIndex;

        int animGroup = pVeh->m_pHandlingData->m_nAnimGroup;
        bool isAllowed = pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE &&
                         (animGroup == ANIMGROUP_TRUCK || animGroup == ANIMGROUP_BUS || animGroup == ANIMGROUP_COACH);
        bool isBigVeh = isAllowed || std::find(ValidForReverseSound.begin(), ValidForReverseSound.end(), pVeh->m_nModelIndex) != ValidForReverseSound.end();

        if (pVeh == FindPlayerVehicle(0, false))
        {
            if (bEngineSounds)
            {
                bool isValid = !CModelInfo::IsPlaneModel(model) && !CModelInfo::IsBmxModel(model) && !CModelInfo::IsHeliModel(model) && !CModelInfo::IsBoatModel(model);
                if (isValid && data.m_bEngineState != pVeh->m_nVehicleFlags.bEngineOn)
                {
                    static std::string carPath = MOD_DATA_PATH("audio/effects/engine_start.wav");
                    static std::string bikePath = MOD_DATA_PATH("audio/effects/bike_engine_start.wav");
                    if (pVeh->m_nVehicleFlags.bEngineOn)
                    {
                        if (CModelInfo::IsCarModel(model))
                        {
                            AudioMgr::LoadAndPlay(&carPath, pVeh, 0.5f);
                        }
                        else
                        {
                            AudioMgr::LoadAndPlay(&bikePath, pVeh, 0.5f);
                        }
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
                    AudioMgr::Play(state ? hOn : hOff, pVeh, 0.5f);
                    data.m_bIndicatorState = state;
                }
            }
        }

        data.m_bEngineState = pVeh->m_nVehicleFlags.bEngineOn;

        CVector vehPos = pVeh->GetPosition();
        CVector camPos = TheCamera.GetPosition();

        float dist = DistanceBetweenPoints(vehPos, camPos);
        if (dist < 80.0f)
        {
            if (bAirbreakSounds && isBigVeh)
            {
                float pedal = pVeh->m_fBreakPedal;
                static float val = 0.0f;

                if (speed > 10.0f)
                {
                    val = std::max(val, pedal);

                    if (pedal >= 0.5f)
                    {
                        data.m_fBrakePressure += pedal * 0.02f;
                    }
                }

                if (pedal <= 0.05f && val != NULL)
                {
                    static std::string path = MOD_DATA_PATH("audio/effects/air_break.wav");
                    static StreamHandle handle = NULL;
                    handle = AudioMgr::Load(&path);
                    AudioMgr::PlayOnVehicle(handle, pVeh, data.m_fBrakePressure);
                    val = NULL;
                    data.m_fBrakePressure = 0.0f;
                }
            }

            if (bReverseSounds)
            {
                static std::string path = MOD_DATA_PATH("audio/effects/reverse.wav");
                if (isBigVeh && pVeh->m_nCurrentGear == 0 && pVeh->m_nVehicleFlags.bEngineOn && !pVeh->m_nVehicleFlags.bEngineBroken && speed >= 3.0f)
                {
                    if (m_hReverse == NULL)
                    {
                        m_hReverse = AudioMgr::Load(&path);
                    }
                    AudioMgr::PlayOnVehicle(m_hReverse, pVeh, 0.5f);
                }
            }
        }
    };
}