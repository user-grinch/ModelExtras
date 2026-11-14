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

static bool IsVehicleEligible(int model, std::string_view jsonKey, bool onlySelected, const std::vector<int> &modelList)
{
    const auto &jsonData = DataMgr::Get(model);
    if (jsonData.is_object() && jsonData.contains("sound") && jsonData["sound"].is_object())
    {
        if (jsonData["sound"].contains(jsonKey) && jsonData["sound"][jsonKey].is_boolean())
        {
            return jsonData["sound"][jsonKey].get<bool>();
        }
        if (jsonKey == "big_vehicle" && jsonData["sound"].contains("reverse_sound") && jsonData["sound"]["reverse_sound"].is_boolean())
        {
            return jsonData["sound"]["reverse_sound"].get<bool>();
        }
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

    std::string brakePadModelsLine = gConfig.ReadString("TABLE", "BrakePad_VehicleModels", "");
    ValidForBrakePad.clear();
    Util::GetModelsFromIni(brakePadModelsLine, ValidForBrakePad);

    bReverseSounds = gConfig.ReadBoolean("SOUND", "ReverseSound", false);
    bEngineSounds = gConfig.ReadBoolean("SOUND", "EngineSound", false);
    bIndicatorSounds = gConfig.ReadBoolean("SOUND", "IndicatorSound", false);
    bAirbreakSounds = gConfig.ReadBoolean("SOUND", "AirbreakSound", false);
    bDoorChimeSounds = gConfig.ReadBoolean("SOUND", "DoorChimeSound", true);
    bDoorChimeOnlySelected = gConfig.ReadBoolean("SOUND", "DoorChimeOnlySelected", true);
    fDoorChimeVolume = std::clamp(gConfig.ReadFloat("SOUND", "DoorChimeVolume", 0.7f), 0.0f, 1.0f);
    bBrakePadSounds = gConfig.ReadBoolean("SOUND", "BrakePadSound", false);
    bBrakePadOnlySelected = gConfig.ReadBoolean("SOUND", "BrakePadOnlySelected", true);
    fBrakePadVolume = std::clamp(gConfig.ReadFloat("SOUND", "BrakePadVolume", 0.7f), 0.0f, 1.0f);
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

static void HandleChimeSounds(CVehicle *pVeh, SoundEffectsData &data, int model, bool isPlayerDriver)
{
    if (!bDoorChimeSounds || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE)
    {
        return;
    }

    if (bOnlyPlayerVehicle && !isPlayerDriver)
    {
        data.m_bDoorChimeActive = false;
        data.m_nLastDoorChimeTime = 0;
        return;
    }

    if (pVeh->bEngineBroken || pVeh->bIsDrowning)
    {
        data.m_bDoorChimeActive = false;
        data.m_nLastDoorChimeTime = 0;
        return;
    }

    if (!IsVehicleEligible(model, "door_chime", bDoorChimeOnlySelected, ValidForDoorChime))
    {
        data.m_bDoorChimeActive = false;
        data.m_nLastDoorChimeTime = 0;
        return;
    }

    CAutomobile *pAuto = static_cast<CAutomobile *>(pVeh);
    bool leftOpen = (!pAuto->m_doors[eDoors::DOOR_FRONT_LEFT].IsClosed() &&
                     std::abs(pAuto->m_doors[eDoors::DOOR_FRONT_LEFT].m_fAngle) > 0.15f);
    bool rightOpen = (!pAuto->m_doors[eDoors::DOOR_FRONT_RIGHT].IsClosed() &&
                      std::abs(pAuto->m_doors[eDoors::DOOR_FRONT_RIGHT].m_fAngle) > 0.15f);

    if (!leftOpen && !rightOpen)
    {
        data.m_bDoorChimeActive = false;
        data.m_nLastDoorChimeTime = 0;
        return;
    }

    data.m_bDoorChimeActive = true;
    unsigned int curTime = CTimer::m_snTimeInMilliseconds;
    if (curTime - data.m_nLastDoorChimeTime >= 1000)
    {
        static std::string chimePath = MOD_DATA_PATH("audio/door_chime.mp3");
        AudioMgr::Play3DSound(chimePath, pVeh->GetPosition(), pVeh, fDoorChimeVolume, 25.0f);
        data.m_nLastDoorChimeTime = curTime;
    }
}

static void HandleBrakePadSounds(CVehicle *pVeh, SoundEffectsData &data, int model, bool isPlayerDriver, bool isBigVeh, float speed)
{
    const auto stopStream = [&]() {
        if (data.m_hBrakePadStream)
        {
            AudioMgr::StopLoopStream(data.m_hBrakePadStream);
            data.m_hBrakePadStream = 0;
            data.m_fBrakePadVol = 0.0f;
        }
    };

    if (!bBrakePadSounds || pVeh->m_nVehicleSubClass != VEHICLE_AUTOMOBILE)
    {
        stopStream();
        return;
    }

    if (bOnlyPlayerVehicle && !isPlayerDriver)
    {
        stopStream();
        return;
    }

    if (isBigVeh || pVeh->bEngineBroken || pVeh->bIsDrowning)
    {
        stopStream();
        return;
    }

    CAutomobile *pAuto = static_cast<CAutomobile *>(pVeh);
    if (pAuto->m_nWheelsOnGround == 0)
    {
        stopStream();
        return;
    }

    bool bWheelLocked = (speed > 5.0f && std::abs(pAuto->m_fWheelSpeed[0]) < 0.01f && std::abs(pAuto->m_fWheelSpeed[1]) < 0.01f);
    if (bWheelLocked)
    {
        stopStream();
        return;
    }

    if (!IsVehicleEligible(model, "brake_pad", bBrakePadOnlySelected, ValidForBrakePad))
    {
        stopStream();
        return;
    }

    float brakeInput = pVeh->m_fBreakPedal;
    float targetVol = 0.0f;
    if (brakeInput > 0.05f && speed > 0.2f)
    {
        float speedFactor = std::clamp((speed - 0.2f) / 4.0f, 0.0f, 1.0f);
        targetVol = fBrakePadVolume * std::clamp(brakeInput, 0.35f, 1.0f) * speedFactor;
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
    else
    {
        stopStream();
    }
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

    bool isMotorVehicle = (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE || pVeh->m_nVehicleSubClass == VEHICLE_MTRUCK);
    bool isBigVeh = isMotorVehicle && IsVehicleEligible(model, "big_vehicle", true, ValidForReverseSound);

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
    HandleChimeSounds(pVeh, data, model, isPlayerDriver);

    // Brake pad friction squeal on braking
    HandleBrakePadSounds(pVeh, data, model, isPlayerDriver, isBigVeh, speed);
}
