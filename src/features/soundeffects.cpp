#include "pch.h"
#include "defines.h"
#include "soundeffects.h"
#include "lights/lights.h"
#include "utils/datamgr.h"
#include "eVehicleClass.h"

using namespace plugin;

std::vector<int> ValidForReverseSound;
std::vector<int> ValidForDoorChime;
std::vector<int> ValidForBrakePad;

#define ANIMGROUP_TRUCK 2
#define ANIMGROUP_BUS 15
#define ANIMGROUP_COACH 16

static bool bReverseSounds = false;
static bool bEngineSounds = false;
static bool bIndicatorSounds = false;
static bool bAirbreakSounds = false;
static bool bDoorChimeSounds = true;
static bool bDoorChimeOnlySelected = true;
static float fDoorChimeVolume = 0.7f;
static bool bBrakePadSounds = false;
static bool bBrakePadOnlySelected = true;
static float fBrakePadVolume = 0.7f;
static bool bOnlyPlayerVehicle = true;

static bool IsVehicleEligible(int model, const char *jsonKey, bool onlySelected, const std::vector<int> &modelList)
{
    const auto &jsonData = DataMgr::Get(model);
    if (jsonData.is_object() && jsonData.contains("sound") && jsonData["sound"].is_object() &&
        jsonData["sound"].contains(jsonKey) && jsonData["sound"][jsonKey].is_boolean())
    {
        return jsonData["sound"][jsonKey].get<bool>();
    }

    if (!onlySelected)
    {
        return true;
    }

    return std::find(modelList.begin(), modelList.end(), model) != modelList.end();
}

void SoundEffects::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    std::string line = gConfig.ReadString("TABLE", "BigVehicleModels", "");
    ValidForReverseSound.clear();
    Util::GetModelsFromIni(line, ValidForReverseSound);

    std::string doorChimeModelsLine = gConfig.ReadString("TABLE", "DoorChime_VehicleModels", "");
    ValidForDoorChime.clear();
    Util::GetModelsFromIni(doorChimeModelsLine, ValidForDoorChime);

    bReverseSounds = gConfig.ReadBoolean("SOUND", "ReverseSound", false);
    bEngineSounds = gConfig.ReadBoolean("SOUND", "EngineSound", false);
    bIndicatorSounds = gConfig.ReadBoolean("SOUND", "IndicatorSound", false);
    bAirbreakSounds = gConfig.ReadBoolean("SOUND", "AirbreakSound", false);
    bDoorChimeSounds = gConfig.ReadBoolean("SOUND", "DoorChimeSound", true);
    bDoorChimeOnlySelected = gConfig.ReadBoolean("SOUND", "DoorChimeOnlySelected", true);
    std::string brakePadModelsLine = gConfig.ReadString("TABLE", "BrakePad_VehicleModels", "");
    ValidForBrakePad.clear();
    Util::GetModelsFromIni(brakePadModelsLine, ValidForBrakePad);

    bBrakePadSounds = gConfig.ReadBoolean("SOUND", "BrakePadSound", false);
    bBrakePadOnlySelected = gConfig.ReadBoolean("SOUND", "BrakePadOnlySelected", true);
    bOnlyPlayerVehicle = !gConfig.ReadBoolean("SOUND", "NonPlayerVehicles", false);
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

}

void SoundEffects::ProcessVehicle(CVehicle *pVeh)
{
    if (!CBaseFeature::IsEnabled(eFeatureMatrix::SoundEffects)) {
        return;
    }
    CPed *pPlayer = FindPlayerPed();
    if (!pPlayer) {
        return;
    }
    if (MathUtil::DistanceSquared(pVeh->GetPosition(), pPlayer->GetPosition()) > (75.0f * 75.0f)) {
        return;
    }

            if (bOnlyPlayerVehicle && pVeh->m_pDriver != pPlayer) {
                return;
            }

            auto &data = m_VehData.Get(pVeh);
            float speed = Util::GetVehicleSpeed(pVeh);
            int model = pVeh->m_nModelIndex;
            bool isPlayerDriver = (pVeh->m_pDriver == pPlayer);

            // Initialize previous state on first detection so newly seen running vehicles don't trigger sound
            if (!data.m_bInitialized)
            {
                data.m_bEngineState = pVeh->bEngineOn;
                data.m_bIndicatorState = Lights::IsIndicatorOn(pVeh);
                data.m_bInitialized = true;
                return;
            }

            int animGroup = pVeh->m_pHandlingData ? pVeh->m_pHandlingData->m_nAnimGroup : 0;
            bool isAllowed = (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_MTRUCK) &&
                            (animGroup == ANIMGROUP_TRUCK || animGroup == ANIMGROUP_BUS || animGroup == ANIMGROUP_COACH ||
                             pVeh->bIsBig || pVeh->bIsBus || (pVeh->bIsVan && pVeh->m_pHandlingData && pVeh->m_pHandlingData->m_fMass >= 2500.0f) ||
                             (pVeh->m_pHandlingData && pVeh->m_pHandlingData->m_fMass >= 3500.0f));
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

            // Door open warning chime: plays a periodic 3D chime when front doors are opened while vehicle is operating
            if (bDoorChimeSounds && pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
            {
                CAutomobile *pAuto = static_cast<CAutomobile *>(pVeh);
                bool isEligible = IsVehicleEligible(model, "door_chime", bDoorChimeOnlySelected, ValidForDoorChime);
                bool isFrontDoorOpen = false;

                if (isEligible && !pVeh->bEngineBroken && !pVeh->bIsDrowning)
                {
                    bool leftOpen = (!pAuto->m_doors[eDoors::DOOR_FRONT_LEFT].IsClosed() &&
                                     std::abs(pAuto->m_doors[eDoors::DOOR_FRONT_LEFT].m_fAngle) > 0.15f);
                    bool rightOpen = (!pAuto->m_doors[eDoors::DOOR_FRONT_RIGHT].IsClosed() &&
                                      std::abs(pAuto->m_doors[eDoors::DOOR_FRONT_RIGHT].m_fAngle) > 0.15f);

                    if (leftOpen || rightOpen)
                    {
                        isFrontDoorOpen = true;
                    }
                }

                bool shouldChime = isFrontDoorOpen && (!bOnlyPlayerVehicle || isPlayerDriver);
                data.m_bDoorChimeActive = shouldChime;

                if (shouldChime)
                {
                    unsigned int curTime = CTimer::m_snTimeInMilliseconds;
                    if (curTime - data.m_nLastDoorChimeTime >= 1000)
                    {
                        static std::string chimePath = MOD_DATA_PATH("audio/door_chime.mp3");
                        AudioMgr::Play3DSound(chimePath, pVeh->GetPosition(), pVeh, fDoorChimeVolume, 25.0f);
                        data.m_nLastDoorChimeTime = curTime;
                    }
                }
                else
                {
                    data.m_nLastDoorChimeTime = 0;
                }
            }

            // Brake pad friction squeal on braking
            if (bBrakePadSounds && pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE && (!bOnlyPlayerVehicle || isPlayerDriver))
            {
                CAutomobile *pAuto = static_cast<CAutomobile *>(pVeh);
                bool isEligible = IsVehicleEligible(model, "brake_pad", bBrakePadOnlySelected, ValidForBrakePad);
                float targetVol = 0.0f;

                bool bOnGround = (pAuto->m_nWheelsOnGround > 0);
                bool bWheelLocked = (speed > 5.0f && std::abs(pAuto->m_fWheelSpeed[0]) < 0.01f && std::abs(pAuto->m_fWheelSpeed[1]) < 0.01f);

                if (isEligible && bOnGround && !bWheelLocked && !pVeh->bEngineBroken && !pVeh->bIsDrowning && !isBigVeh && !pVeh->bIsBig && !pVeh->bIsBus)
                {
                    float brakeInput = pVeh->m_fBreakPedal;
                    if (brakeInput > 0.05f && speed > 0.2f)
                    {
                        float speedFactor = std::clamp((speed - 0.2f) / 4.0f, 0.0f, 1.0f);
                        targetVol = fBrakePadVolume * std::clamp(brakeInput, 0.35f, 1.0f) * speedFactor;
                    }
                }

                if (targetVol > data.m_fBrakePadVol)
                {
                    data.m_fBrakePadVol = std::min(targetVol, data.m_fBrakePadVol + 0.1f);
                }
                else if (targetVol < data.m_fBrakePadVol)
                {
                    data.m_fBrakePadVol = std::max(targetVol, data.m_fBrakePadVol - 0.06f);
                }

                if (data.m_fBrakePadVol > 0.01f)
                {
                    static std::string brakePadPath = MOD_DATA_PATH("audio/brake_pad.mp3");
                    if (!data.m_hBrakePadStream)
                    {
                        data.m_hBrakePadStream = AudioMgr::PlayLoopStream(brakePadPath, pVeh->GetPosition(), data.m_fBrakePadVol, 25.0f);
                    }
                    else
                    {
                        AudioMgr::UpdateLoopStream(data.m_hBrakePadStream, pVeh->GetPosition(), data.m_fBrakePadVol, 25.0f);
                    }
                }
                else if (data.m_hBrakePadStream)
                {
                    AudioMgr::StopLoopStream(data.m_hBrakePadStream);
                    data.m_fBrakePadVol = 0.0f;
                }
            }
}