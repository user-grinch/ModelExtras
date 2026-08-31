#include "pch.h"
#include "defines.h"
#include "soundeffects.h"
#include "lights.h"
#include "eVehicleClass.h"

using namespace plugin;

std::vector<int> ValidForReverseSound;

#define ANIMGROUP_TRUCK 2
#define ANIMGROUP_BUS 15
#define ANIMGROUP_COACH 16

static bool bReverseSounds = false;
static bool bEngineSounds = false;
static bool bIndicatorSounds = false;
static bool bAirbreakSounds = false;
static bool bOnlyPlayerVehicle = true;

void SoundEffects::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    std::string line = gConfig.ReadString("TABLE", "SoundEffects_BigVehicleModels", "");
    ValidForReverseSound.clear();
    Util::GetModelsFromIni(line, ValidForReverseSound);

    bReverseSounds = gConfig.ReadBoolean("SOUND", "SoundEffects_GlobalReverseSound", gConfig.ReadBoolean("FEATURES", "SoundEffects_GlobalReverseSound", false));
    bEngineSounds = gConfig.ReadBoolean("SOUND", "SoundEffects_GlobalEngineSound", gConfig.ReadBoolean("FEATURES", "SoundEffects_GlobalEngineSound", false));
    bIndicatorSounds = gConfig.ReadBoolean("SOUND", "SoundEffects_GlobalIndicatorSound", gConfig.ReadBoolean("FEATURES", "SoundEffects_GlobalIndicatorSound", false));
    bAirbreakSounds = gConfig.ReadBoolean("SOUND", "SoundEffects_GlobalAirbreakSound", gConfig.ReadBoolean("FEATURES", "SoundEffects_GlobalAirbreakSound", false));
    bOnlyPlayerVehicle = !gConfig.ReadBoolean("SOUND", "SoundEffects_NonPlayerVehicles", gConfig.ReadBoolean("FEATURES", "SoundEffects_NonPlayerVehicles", false));
}

void SoundEffects::Reload(CVehicle *pVeh)
{
    ReloadConfig();
}

void SoundEffects::Init()
{
    ReloadConfig();

    Events::initGameEvent += [this]()
    {
        ReloadConfig();
    };

    Events::processScriptsEvent += []() {
        if (!CBaseFeature::IsEnabled(eFeatureMatrix::SoundEffects)) {
            return;
        }
        CPed *pPlayer = FindPlayerPed();
        if (!pPlayer) {
            return;
        }
        CVector playerPos = pPlayer->GetPosition();

		for (CVehicle *pVeh : CPools::ms_pVehiclePool) {
			if (CVector::Distance(pVeh->GetPosition(), playerPos) > 75.0f ) {
				continue;
			}

            if (bOnlyPlayerVehicle && pVeh->m_pDriver != FindPlayerPed()) {
                continue;
            }

            auto &data = m_VehData.Get(pVeh);
            float speed = Util::GetVehicleSpeed(pVeh);
            int model = pVeh->m_nModelIndex;
            bool isPlayerDriver = (pVeh->m_pDriver == FindPlayerPed());

            // Initialize previous state on first detection so newly seen running vehicles don't trigger sound
            if (!data.m_bInitialized)
            {
                data.m_bEngineState = pVeh->bEngineOn;
                data.m_bIndicatorState = Lights::IsIndicatorOn(pVeh);
                data.m_bInitialized = true;
                continue;
            }

            int animGroup = pVeh->m_pHandlingData ? pVeh->m_pHandlingData->m_nAnimGroup : 0;
            bool isAllowed = pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE &&
                            (animGroup == ANIMGROUP_TRUCK || animGroup == ANIMGROUP_BUS || animGroup == ANIMGROUP_COACH ||
                             pVeh->bIsBig || pVeh->bIsBus || pVeh->bIsVan || (pVeh->m_pHandlingData && pVeh->m_pHandlingData->m_fMass >= 3500.0f));
            bool isBigVeh = isAllowed || std::find(ValidForReverseSound.begin(), ValidForReverseSound.end(), pVeh->m_nModelIndex) != ValidForReverseSound.end();

            if (bEngineSounds)
            {
                bool isValid = !CModelInfo::IsPlaneModel(model) && !CModelInfo::IsBmxModel(model) && !CModelInfo::IsHeliModel(model) && !CModelInfo::IsBoatModel(model);
                bool isEligible = isPlayerDriver || (!bOnlyPlayerVehicle && pVeh->m_pDriver != nullptr);

                if (isValid && isEligible)
                {
                    // Engine transitioned from OFF (false) to ON (true)
                    if (!data.m_bEngineState && pVeh->bEngineOn)
                    {
                        unsigned int curTime = CTimer::m_snTimeInMilliseconds;
                        if (curTime - data.m_nLastEngineSoundTime > 2000)
                        {
                            static std::string carPath = MOD_DATA_PATH("audio/engine_start.wav");
                            static std::string bikePath = MOD_DATA_PATH("audio/bike_engine_start.wav");
                            if (CModelInfo::IsBikeModel(model) || CModelInfo::IsQuadBikeModel(model))
                            {
                                AudioMgr::Play3DSound(bikePath, pVeh->GetPosition(), pVeh, 1.0f, 65.0f);
                            }
                            else
                            {
                                AudioMgr::Play3DSound(carPath, pVeh->GetPosition(), pVeh, 1.0f, 65.0f);
                            }
                            data.m_nLastEngineSoundTime = curTime;
                        }
                    }
                }

                data.m_bEngineState = pVeh->bEngineOn;
            }

            if (bIndicatorSounds && isPlayerDriver)
            {
                bool state = Lights::IsIndicatorOn(pVeh);
                if (state != data.m_bIndicatorState)
                {
                    static std::string onpath = MOD_DATA_PATH("audio/indicator_on.wav");
                    static std::string offpath = MOD_DATA_PATH("audio/indicator_off.wav");

                    if (state)
                    {
                        AudioMgr::PlayFileSound(onpath, 0.6f);
                    }
                    else
                    {
                        AudioMgr::PlayFileSound(offpath, 0.6f);
                    }
                    data.m_bIndicatorState = state;
                }
            }

            if (bAirbreakSounds && isBigVeh)
            {
                float pedal = pVeh->m_fBreakPedal;

                if (speed > 10.0f)
                {
                    data.m_fMaxPedal = std::max(data.m_fMaxPedal, pedal);

                    if (pedal >= 0.5f)
                    {
                        data.m_fBrakePressure += pedal * 0.02f;
                    }
                }

                if (pedal <= 0.05f && data.m_fMaxPedal > 0.0f)
                {
                    static std::string path = MOD_DATA_PATH("audio/airbreak.wav");
                    AudioMgr::Play3DSound(path, pVeh->GetPosition(), pVeh, data.m_fBrakePressure, 60.0f);
                    data.m_fMaxPedal = 0.0f;
                    data.m_fBrakePressure = 0.0f;
                }
            }

            if (bReverseSounds)
            {
                static std::string path = MOD_DATA_PATH("audio/reverse.wav");

                if (isBigVeh && pVeh->m_nCurrentGear == 0 && pVeh->bEngineOn && !pVeh->bEngineBroken && speed >= 3.0f)
                {
                    unsigned int curTime = CTimer::m_snTimeInMilliseconds;
                    if (curTime - data.m_nLastReverseSoundTime >= 1000)
                    {
                        AudioMgr::Play3DSound(path, pVeh->GetPosition(), pVeh, 0.7f, 50.0f);
                        data.m_nLastReverseSoundTime = curTime;
                    }
                }
            }
		}
	};
}