#include "pch.h"
#include "defines.h"
#include "soundeffects.h"
#include "lights/lights.h"
#include "eVehicleClass.h"

#include "utils/datamgr.h"

using namespace plugin;

std::vector<int> ValidForReverseSound;
static std::vector<int> ValidForDoorChime;

#define ANIMGROUP_TRUCK 2
#define ANIMGROUP_BUS 15
#define ANIMGROUP_COACH 16

static bool bReverseSounds = false;
static bool bEngineSounds = false;
static bool bIndicatorSounds = false;
static bool bAirbreakSounds = false;
static bool bOnlyPlayerVehicle = true;
static bool bDoorChimeSounds = true;
static bool bDoorChimeOnlySelected = true;
static float fDoorChimeVolume = 0.7f;

static std::string GetDoorChimeAudioPath()
{
    const char *extensions[] = {".mp3", ".wav", ".ogg"};
    for (const char *ext : extensions)
    {
        std::string relPath = std::string("ModelExtras/audio/door_chime") + ext;
        std::string fullPath = PLUGIN_PATH((char *)relPath.c_str());
        DWORD attr = GetFileAttributesA(fullPath.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            return fullPath;
        }
    }
    return MOD_DATA_PATH("audio/door_chime.mp3");
}

static bool IsVehicleEligibleForDoorChime(int model)
{
    const auto &jsonData = DataMgr::Get(model);
    if (jsonData.contains("sound") && jsonData["sound"].contains("door_chime"))
    {
        return jsonData["sound"]["door_chime"].get<bool>();
    }

    if (!bDoorChimeOnlySelected)
    {
        return true;
    }

    if (!ValidForDoorChime.empty() &&
        std::find(ValidForDoorChime.begin(), ValidForDoorChime.end(), model) != ValidForDoorChime.end())
    {
        return true;
    }

    return false;
}

void SoundEffects::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    std::string line = gConfig.ReadString("TABLE", "BigVehicleModels", "");
    ValidForReverseSound.clear();
    Util::GetModelsFromIni(line, ValidForReverseSound);

    std::string doorChimeModelsLine = gConfig.ReadString("TABLE", "DoorChime_VehicleModels", "400, 401, 402, 405, 409, 410, 411, 413, 415, 416, 418, 420, 421, 422, 426, 429, 436, 440, 445, 451, 458, 459, 470, 477, 480, 482, 489, 490, 492, 496, 500, 505, 506, 507, 516, 517, 526, 527, 529, 533, 540, 541, 546, 547, 550, 551, 554, 558, 559, 560, 561, 562, 565, 579, 582, 585, 587, 589, 596, 597, 598, 599, 602, 603");
    ValidForDoorChime.clear();
    Util::GetModelsFromIni(doorChimeModelsLine, ValidForDoorChime);

    bReverseSounds = gConfig.ReadBoolean("SOUND", "GlobalReverseSound", false);
    bEngineSounds = gConfig.ReadBoolean("SOUND", "GlobalEngineSound", false);
    bIndicatorSounds = gConfig.ReadBoolean("SOUND", "GlobalIndicatorSound", false);
    bAirbreakSounds = gConfig.ReadBoolean("SOUND", "GlobalAirbreakSound", false);
    bOnlyPlayerVehicle = !gConfig.ReadBoolean("SOUND", "NonPlayerVehicles", false);
    bDoorChimeSounds = gConfig.ReadBoolean("SOUND", "DoorChimeSound", true);
    bDoorChimeOnlySelected = gConfig.ReadBoolean("SOUND", "DoorChimeOnlySelected", true);
    fDoorChimeVolume = std::clamp(gConfig.ReadFloat("SOUND", "DoorChimeVolume", 0.7f), 0.0f, 1.0f);
}

void SoundEffects::Reload(CVehicle *pVeh)
{
    ReloadConfig();
    if (pVeh)
    {
        auto &data = m_VehData.Get(pVeh);
        if (data.m_hDoorChimeStream)
        {
            AudioMgr::StopLoopStream(data.m_hDoorChimeStream);
        }
    }
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

            if (bOnlyPlayerVehicle && pVeh->m_pDriver != FindPlayerPed()) {
                return;
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

            if (bDoorChimeSounds && pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE)
            {
                CAutomobile *pAuto = static_cast<CAutomobile *>(pVeh);
                bool isEligible = IsVehicleEligibleForDoorChime(model);
                bool isFrontDoorOpen = false;

                if (isEligible && !pVeh->bEngineBroken && !pVeh->bIsDrowning)
                {
                    bool leftOpen = (!pAuto->m_doors[eDoors::DOOR_FRONT_LEFT].IsClosed() &&
                                     std::abs(pAuto->m_doors[eDoors::DOOR_FRONT_LEFT].m_fAngle) > 0.15f);
                    bool rightOpen = (!pAuto->m_doors[eDoors::DOOR_FRONT_RIGHT].IsClosed() &&
                                      std::abs(pAuto->m_doors[eDoors::DOOR_FRONT_RIGHT].m_fAngle) > 0.15f);

                    if (leftOpen || rightOpen)
                    {
                        if (isPlayerDriver || pVeh->bEngineOn || (pPlayer && MathUtil::DistanceSquared(pVeh->GetPosition(), pPlayer->GetPosition()) < (12.0f * 12.0f)))
                        {
                            isFrontDoorOpen = true;
                        }
                    }
                }

                if (isFrontDoorOpen)
                {
                    static std::string chimePath = GetDoorChimeAudioPath();
                    if (!data.m_hDoorChimeStream)
                    {
                        data.m_hDoorChimeStream = AudioMgr::PlayLoopStream(chimePath, pVeh->GetPosition(), fDoorChimeVolume, 25.0f);
                    }
                    else
                    {
                        AudioMgr::UpdateLoopStream(data.m_hDoorChimeStream, pVeh->GetPosition(), fDoorChimeVolume, 25.0f);
                    }
                }
                else
                {
                    if (data.m_hDoorChimeStream)
                    {
                        AudioMgr::StopLoopStream(data.m_hDoorChimeStream);
                    }
                }
            }

}