#include "pch.h"
#include "ModelExtrasAPI.h"
#include "defines.h"
#include "loader.h"
#include "utils/datamgr.h"
#include "utils/modelinfomgr.h"
#include "utils/car.h"
#include "utils/render.h"
#include "features/sirens.h"
#include "features/lights/data.h"
#include "features/lights/manager.h"
#include "features/lights/components/headlight.h"
#include "features/plate.h"
#include "features/carcols.h"
#include "features/dirtfx.h"
#include "features/remap.h"
#include "features/roof.h"
#include "features/spoiler.h"
#include "features/rollbackbed.h"
#include "features/spotlights.h"
#include "features/gauge.h"
#include "features/leds.h"
#include "features/exhausts.h"
#include "features/backfire.h"
#include "features/pedcols.h"
#include "features/soundeffects.h"
#include "features/slidedoor.h"
#include "features/rotatedoor.h"
#include "features/wheel.h"
#include "features/wheelhub.h"
#include "features/chain.h"
#include "features/clock.h"
#include "enums/lightingmode.h"
#include "utils/audiomgr.h"
#include <algorithm>
#include <cstring>

namespace {
static void SafeStrCopy(char *dst, int maxLen, const std::string &src) {
  if (dst && maxLen > 0) {
    strncpy_s(dst, maxLen, src.c_str(), _TRUNCATE);
  }
}
}

extern "C" {

// Core & Lifecycle
int ME_GetAPIVersion() { return ME_API_VERSION; }

int ME_GetVersion() { return MOD_VERSION_NUMBER; }

bool ME_IsFeatureAvail(ME_FeatureID featureId) {
  auto idx = static_cast<int>(featureId);
  if (idx < 0 || idx >= static_cast<int>(eFeatureMatrix::FeatureCount)) {
    return false;
  }
  return ModelExtras::m_bEnabledFeatures.test(idx);
}

bool ME_IsFeatureEnabled(ME_FeatureID featureId) {
  auto idx = static_cast<int>(featureId);
  if (idx < 0 || idx >= static_cast<int>(eFeatureMatrix::FeatureCount)) {
    return false;
  }
  return CBaseFeature::IsEnabled(static_cast<eFeatureMatrix>(idx));
}

void ME_Reload() {
  ModelExtras::Reload();
}

void ME_ReloadVehicle(CVehicle *pVeh) {
  if (pVeh) {
    for (const auto &pFeature : ModelExtras::m_Features) {
      if (pFeature) {
        pFeature->Reload(pVeh);
      }
    }
    ModelInfoMgr::Reload(pVeh);
  }
}

// Dummy function to show on crash logs
int __declspec(dllexport) ignore1(int i) { return 1; }

// Vehicle Lights API
bool ME_GetVehicleLightState(CVehicle *pVeh, ME_LightID lightId) {
  if (!pVeh) return false;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return data.bLightStates[matType];
}

void ME_SetVehicleLightState(CVehicle *pVeh, ME_LightID lightId, bool state) {
  if (!pVeh) return;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return;
  auto &data = LightManager::m_VehData.Get(pVeh);
  data.bLightStates[matType] = state;
  if (state) {
    ModelInfoMgr::EnableMaterial(pVeh, matType);
  }
}

bool ME_IsLightAvailable(CVehicle *pVeh, ME_LightID lightId) {
  if (!pVeh) return false;
  auto matType = static_cast<eMaterialType>(lightId);
  return ModelInfoMgr::IsMaterialAvailable(pVeh, matType);
}

int ME_GetIndicatorState(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return static_cast<int>(data.nIndicatorState);
}

void ME_SetIndicatorState(CVehicle *pVeh, int state) {
  if (!pVeh) return;
  if (state < 0 || state > 3) return;
  auto &data = LightManager::m_VehData.Get(pVeh);
  data.nIndicatorState = static_cast<eIndicatorState>(state);
}

int ME_GetLightOverride(CVehicle *pVeh) {
  if (!pVeh) return 0;
  return pVeh->m_nOverrideLights;
}

void ME_SetLightOverride(CVehicle *pVeh, int overrideMode) {
  if (!pVeh) return;
  if (overrideMode >= 0 && overrideMode <= 2) {
    pVeh->m_nOverrideLights = static_cast<unsigned char>(overrideMode);
  }
}

bool ME_GetFogLights(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return data.bFogLightsOn;
}

void ME_SetFogLights(CVehicle *pVeh, bool on) {
  if (!pVeh) return;
  auto &data = LightManager::m_VehData.Get(pVeh);
  data.bFogLightsOn = on;
  if (on) {
    ModelInfoMgr::EnableMaterial(pVeh, eMaterialType::FogLightLeft);
    ModelInfoMgr::EnableMaterial(pVeh, eMaterialType::FogLightRight);
  }
}

bool ME_GetHighBeam(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return data.bLongLightsOn;
}

void ME_SetHighBeam(CVehicle *pVeh, bool on) {
  if (!pVeh) return;
  auto &data = LightManager::m_VehData.Get(pVeh);
  data.bLongLightsOn = on;
  data.fHighBeamFactor = on ? 1.0f : 0.0f;
}

float ME_GetLightInertia(CVehicle *pVeh, ME_LightID lightId) {
  if (!pVeh) return 0.0f;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return 0.0f;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return LightManager::GetLightInertia(pVeh, data, matType);
}

bool ME_GetLightColor(CVehicle *pVeh, ME_LightID lightId, ME_Color *onColor, ME_Color *offColor) {
  if (!pVeh) return false;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return false;
  MatStateColor col = LightManager::GetMaterialColor(pVeh, matType);
  if (onColor) *onColor = {col.on.r, col.on.g, col.on.b, col.on.a};
  if (offColor) *offColor = {col.off.r, col.off.g, col.off.b, col.off.a};
  return true;
}

float ME_GetLightFactor(CVehicle *pVeh, ME_LightID lightId) {
  if (!pVeh) return 0.0f;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return 0.0f;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return data.fLightFactor[matType];
}

int ME_GetLightDummyCount(CVehicle *pVeh, ME_LightID lightId) {
  if (!pVeh) return 0;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return 0;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return static_cast<int>(data.dummies[matType].size());
}

bool ME_GetLightDummyData(CVehicle *pVeh, ME_LightID lightId, int dummyIndex, ME_LightDummyInfo *outInfo) {
  if (!pVeh || !outInfo || dummyIndex < 0) return false;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  if (dummyIndex >= static_cast<int>(data.dummies[matType].size())) return false;
  const auto &c = data.dummies[matType][dummyIndex]->GetRef();
  outInfo->pFrame = c.frame;
  outInfo->pos = c.position;
  outInfo->color = {c.corona.color.r, c.corona.color.g, c.corona.color.b, c.corona.color.a};
  outInfo->fSize = c.corona.size;
  outInfo->fAngle = c.rotation.angle;
  outInfo->lightingType = static_cast<int>(c.corona.lightingType);
  return true;
}

bool ME_IsLightDamaged(CVehicle *pVeh, ME_LightID lightId, int dummyIndex) {
  if (!pVeh) return false;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  if (dummyIndex >= 0 && dummyIndex < static_cast<int>(data.dummies[matType].size())) {
    return CarUtil::IsDummyDamaged(pVeh, data.dummies[matType][dummyIndex]->GetRef());
  }
  if (matType == eMaterialType::HeadLightLeft) return CarUtil::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_LEFT);
  if (matType == eMaterialType::HeadLightRight) return CarUtil::IsLightDamaged(pVeh, eLights::LIGHT_FRONT_RIGHT);
  if (matType == eMaterialType::TailLightLeft || matType == eMaterialType::BrakeLightLeft) return CarUtil::IsLightDamaged(pVeh, eLights::LIGHT_REAR_LEFT);
  if (matType == eMaterialType::TailLightRight || matType == eMaterialType::BrakeLightRight) return CarUtil::IsLightDamaged(pVeh, eLights::LIGHT_REAR_RIGHT);
  return false;
}

bool ME_ArePopUpHeadlightsOpen(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return HeadlightComponent::AreHeadlightsOpen(pVeh, data);
}

bool ME_HasPopUpHeadlights(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  if (pVeh->m_nModelIndex == MODEL_ZR350) return true;
  return data.bHasVehFuncsPopUp;
}

bool ME_IsLightRendered(CVehicle *pVeh, ME_LightID lightId) {
  if (!pVeh) return false;
  auto matType = static_cast<eMaterialType>(lightId);
  if (matType < 0 || matType >= eMaterialType::TotalMaterial) return false;
  auto &data = LightManager::m_VehData.Get(pVeh);
  return data.bLightRenderedThisFrame[matType];
}

float ME_GetRealisticSpeed(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  return CarUtil::GetVehicleSpeedRealistic(pVeh);
}

// Sirens API
int ME_GetSirenStateCount(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto *pModelData = Sirens::GetModelData(pVeh->m_nModelIndex);
  if (!pModelData) return 0;
  return static_cast<int>(pModelData->States.size());
}

int ME_GetSirenStateCountByModel(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return 0;
  auto *pModelData = Sirens::GetModelData(modelIndex);
  if (!pModelData) return 0;
  return static_cast<int>(pModelData->States.size());
}

int ME_GetSirenState(CVehicle *pVeh) {
  if (!pVeh) return -1;
  auto *pModelData = Sirens::GetModelData(pVeh->m_nModelIndex);
  if (!pModelData || pModelData->States.empty()) return -1;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return -1;
  return pData->State;
}

bool ME_SetSirenState(CVehicle *pVeh, int state) {
  if (!pVeh) return false;
  auto *pModelData = Sirens::GetModelData(pVeh->m_nModelIndex);
  if (!pModelData || pModelData->States.empty()) return false;
  if (state < 0 || state >= static_cast<int>(pModelData->States.size())) return false;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return false;
  pData->vehicle = pVeh;
  pData->State = state;
  return true;
}

int ME_GetSirenSoundMode(CVehicle *pVeh) {
  if (!pVeh) return -1;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return -1;
  return pData->SoundMode;
}

bool ME_SetSirenSoundMode(CVehicle *pVeh, int soundMode) {
  if (!pVeh) return false;
  if (soundMode < 0 || soundMode > 8) return false;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return false;
  pData->vehicle = pVeh;
  pData->SoundMode = soundMode;
  return true;
}

bool ME_GetSirenMute(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return false;
  return pData->Mute;
}

void ME_SetSirenMute(CVehicle *pVeh, bool mute) {
  if (!pVeh) return;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return;
  pData->vehicle = pVeh;
  pData->Mute = mute;
  if (mute) {
    pVeh->bSirenOrAlarm = false;
  }
}

bool ME_IsSirenActive(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return false;
  return pData->GetSirenState();
}

bool ME_IsSirenVehicle(CVehicle *pVeh) {
  if (!pVeh) return false;
  return Sirens::IsSirenModel(pVeh->m_nModelIndex);
}

bool ME_IsCustomSirenPlaying(CVehicle *pVeh) {
  if (!pVeh) return false;
  return Sirens::IsPlayingCustomSiren(pVeh);
}

bool ME_HasSirens(CVehicle *pVeh) {
  if (!pVeh) return false;
  return Sirens::IsSirenModel(pVeh->m_nModelIndex);
}

int ME_GetSirenSoundState(CVehicle *pVeh) {
  if (!pVeh) return -1;
  auto *pData = Sirens::GetVehicleData(pVeh);
  if (!pData) return -1;
  return pData->m_nActiveSirenSoundState;
}

// License Plate API
bool ME_GetPlateText(CVehicle *pVeh, char *outBuffer, int maxLen) {
  if (!pVeh || !outBuffer || maxLen <= 0) return false;
  auto &data = LicensePlate::m_VehData.Get(pVeh);
  if (!data.m_szLastPlateText.empty()) {
    strncpy_s(outBuffer, maxLen, data.m_szLastPlateText.c_str(), _TRUNCATE);
    return true;
  }
  return false;
}

bool ME_SetPlateText(CVehicle *pVeh, const char *text) {
  if (!pVeh || !text) return false;
  auto &data = LicensePlate::m_VehData.Get(pVeh);
  data.m_szLastPlateText = text;
  return true;
}

bool ME_HasCustomPlate(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = LicensePlate::m_VehData.Get(pVeh);
  return data.m_pCustomPlateTex != nullptr || !data.m_szLastPlateText.empty();
}

bool ME_GetPlateColors(CVehicle *pVeh, ME_Color *onColor, ME_Color *offColor) {
  if (!pVeh) return false;
  if (!DataMgr::Has(pVeh->m_nModelIndex)) return false;
  const auto &json = DataMgr::Get(pVeh->m_nModelIndex);
  const nlohmann::json *pSec = nullptr;
  if (json.contains("plate")) pSec = &json["plate"];
  else if (json.contains("license_plate")) pSec = &json["license_plate"];
  else if (json.contains("lights")) {
    if (json["lights"].contains("plate")) pSec = &json["lights"]["plate"];
    else if (json["lights"].contains("license_plate")) pSec = &json["lights"]["license_plate"];
  }
  if (!pSec) return false;

  auto parseCol = [](const nlohmann::json &sec, const char *key, ME_Color *outCol) -> bool {
    if (!outCol) return false;
    const nlohmann::json *val = nullptr;
    if (sec.contains(key)) val = &sec[key];
    else if (sec.contains("material") && sec["material"].contains(key)) val = &sec["material"][key];
    if (!val) return false;
    if (val->is_array() && val->size() >= 3) {
      outCol->r = (*val)[0].get<uint8_t>();
      outCol->g = (*val)[1].get<uint8_t>();
      outCol->b = (*val)[2].get<uint8_t>();
      outCol->a = val->size() >= 4 ? (*val)[3].get<uint8_t>() : 255;
      return true;
    }
    if (val->is_object()) {
      outCol->r = val->value("red", val->value("r", 255));
      outCol->g = val->value("green", val->value("g", 255));
      outCol->b = val->value("blue", val->value("b", 255));
      outCol->a = val->value("alpha", val->value("a", 255));
      return true;
    }
    return false;
  };

  bool foundOn = parseCol(*pSec, "color", onColor);
  bool foundOff = parseCol(*pSec, "color_off", offColor);
  return foundOn || foundOff;
}

int ME_GetPlateCity(CVehicle *pVeh) {
  if (!pVeh) return -1;
  auto &data = LicensePlate::m_VehData.Get(pVeh);
  return data.cityId;
}

void ME_SetPlateCity(CVehicle *pVeh, int cityId) {
  if (!pVeh) return;
  auto &data = LicensePlate::m_VehData.Get(pVeh);
  data.cityId = cityId;
}

bool ME_IsPlateNightIlluminated(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = LicensePlate::m_VehData.Get(pVeh);
  return data.m_bNightTexture;
}

// Carcols & Colors API
bool ME_GetVehicleColors(CVehicle *pVeh, ME_Color *col1, ME_Color *col2, ME_Color *col3, ME_Color *col4) {
  if (!pVeh) return false;
  auto &data = Carcols::m_VehData.Get(pVeh);
  if (col1) *col1 = {data.m_Colors.primary.r, data.m_Colors.primary.g, data.m_Colors.primary.b, data.m_Colors.primary.a};
  if (col2) *col2 = {data.m_Colors.secondary.r, data.m_Colors.secondary.g, data.m_Colors.secondary.b, data.m_Colors.secondary.a};
  if (col3) *col3 = {data.m_Colors.tert.r, data.m_Colors.tert.g, data.m_Colors.tert.b, data.m_Colors.tert.a};
  if (col4) *col4 = {data.m_Colors.quart.r, data.m_Colors.quart.g, data.m_Colors.quart.b, data.m_Colors.quart.a};
  return true;
}

bool ME_SetVehicleColors(CVehicle *pVeh, ME_Color col1, ME_Color col2, ME_Color col3, ME_Color col4) {
  if (!pVeh) return false;
  auto &data = Carcols::m_VehData.Get(pVeh);
  data.m_Colors.primary = {col1.r, col1.g, col1.b, col1.a};
  data.m_Colors.secondary = {col2.r, col2.g, col2.b, col2.a};
  data.m_Colors.tert = {col3.r, col3.g, col3.b, col3.a};
  data.m_Colors.quart = {col4.r, col4.g, col4.b, col4.a};
  data.m_bPri = data.m_bSec = data.m_bTer = data.m_bQuat = true;
  return true;
}

int ME_GetCarcolVariationCount(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return 0;
  auto jsonData = DataMgr::Get(modelIndex);
  if (jsonData.contains("carcols") && jsonData["carcols"].is_array()) {
    return static_cast<int>(jsonData["carcols"].size());
  }
  return 0;
}

int ME_GetCarcolVariation(CVehicle *pVeh) {
  if (!pVeh) return -1;
  auto &data = Carcols::m_VehData.Get(pVeh);
  return data.randId;
}

bool ME_SetCarcolVariation(CVehicle *pVeh, int variationIndex) {
  if (!pVeh || variationIndex < 0) return false;
  auto &data = Carcols::m_VehData.Get(pVeh);
  data.randId = variationIndex;
  return true;
}

// Exhausts & Nitro API
unsigned int ME_GetExhaustCount(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = ExhaustFx::m_VehData.Get(pVeh);
  return static_cast<unsigned int>(data.m_pDummies.size());
}

ME_ExhaustInfo ME_GetExhaustData(CVehicle *pVeh, int index) {
  ME_ExhaustInfo info{};
  if (!pVeh) return info;
  auto &data = ExhaustFx::m_VehData.Get(pVeh);
  if (index >= 0 && index < static_cast<int>(data.m_pDummies.size())) {
    const auto &ex = data.m_pDummies[index].second;
    info.pFrame = ex.pFrame;
    info.Color = {ex.Color.r, ex.Color.g, ex.Color.b, ex.Color.a};
    info.fSpeedMul = ex.fSpeedMul;
    info.fLifeTime = ex.fLifeTime;
    info.fSizeMul = ex.fSizeMul;
    info.bNitroEffect = ex.bNitroEffect;
  }
  return info;
}

void ME_SetExhaustData(CVehicle *pVeh, int index, ME_ExhaustInfo &data) {
  if (!pVeh) return;
  auto &vData = ExhaustFx::m_VehData.Get(pVeh);
  if (index >= 0 && index < static_cast<int>(vData.m_pDummies.size())) {
    auto &ex = vData.m_pDummies[index].second;
    ex.pFrame = data.pFrame;
    ex.Color = {data.Color.r, data.Color.g, data.Color.b, data.Color.a};
    ex.fSpeedMul = data.fSpeedMul;
    ex.fLifeTime = data.fLifeTime;
    ex.fSizeMul = data.fSizeMul;
    ex.bNitroEffect = data.bNitroEffect;
  }
}

void ME_TriggerNitro(CVehicle *pVeh, bool enable) {
  if (!pVeh) return;
  if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) {
    auto *pAuto = reinterpret_cast<CAutomobile *>(pVeh);
    pAuto->m_fNitroValue = enable ? -1.0f : 0.0f;
  }
}

bool ME_IsNitroFiring(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = ExhaustFx::m_VehData.Get(pVeh);
  return (CTimer::m_FrameCounter - data.lastNitroFrame) <= 1;
}

// Backfire API
void ME_TriggerBackfire(CVehicle *pVeh, bool bPlaySound) {
  if (!pVeh) return;
  BackFireEffect::BackFireSingle(pVeh, bPlaySound);
}

void ME_TriggerBackfireBurst(CVehicle *pVeh, bool bPlaySound) {
  if (!pVeh) return;
  BackFireEffect::BackFireMulti(pVeh, bPlaySound);
}

bool ME_IsBackfireActive(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = BackFireEffect::m_VehData.Get(pVeh);
  return data.m_nleftFires > 0;
}

// Dashboard, Gauges & Clock API
float ME_GetVehicleMileage(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  auto &data = MileageIndicator::m_VehData.Get(pVeh);
  if (!data.vecIndicatorData.empty()) {
    return static_cast<float>(data.vecIndicatorData.begin()->second.dCurrentDistance);
  }
  return 0.0f;
}

void ME_SetVehicleMileage(CVehicle *pVeh, float mileageKm) {
  if (!pVeh) return;
  auto &data = MileageIndicator::m_VehData.Get(pVeh);
  for (auto &pair : data.vecIndicatorData) {
    pair.second.dCurrentDistance = mileageKm;
  }
}

bool ME_HasOdometer(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = MileageIndicator::m_VehData.Get(pVeh);
  return !data.vecIndicatorData.empty();
}

float ME_GetSpeedometerValue(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  auto &data = SpeedGauge::m_VehData.Get(pVeh);
  if (!data.vecGaugeData.empty()) {
    return data.vecGaugeData.begin()->second.fCurRotation;
  }
  return 0.0f;
}

bool ME_HasSpeedometer(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = SpeedGauge::m_VehData.Get(pVeh);
  return !data.vecGaugeData.empty();
}

float ME_GetRPMValue(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  auto &data = RPMGauge::m_VehData.Get(pVeh);
  if (!data.vecGaugeData.empty()) {
    return data.vecGaugeData.begin()->second.fCurRotation;
  }
  return 0.0f;
}

bool ME_HasRPMGauge(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = RPMGauge::m_VehData.Get(pVeh);
  return !data.vecGaugeData.empty();
}

float ME_GetTurboValue(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  auto &data = TurboGauge::m_VehData.Get(pVeh);
  if (!data.vecGaugeData.empty()) {
    return data.vecGaugeData.begin()->second.fCurRotation;
  }
  return 0.0f;
}

bool ME_HasTurboGauge(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = TurboGauge::m_VehData.Get(pVeh);
  return !data.vecGaugeData.empty();
}

bool ME_HasGearIndicator(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = GearIndicator::m_VehData.Get(pVeh);
  return !data.vecIndicatorData.empty();
}

int ME_GetGearIndicatorValue(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = GearIndicator::m_VehData.Get(pVeh);
  if (!data.vecIndicatorData.empty()) {
    return static_cast<int>(data.vecIndicatorData[0].iCurrent);
  }
  return 0;
}

void ME_SetGearIndicatorValue(CVehicle *pVeh, int gear) {
  if (!pVeh || gear < 0) return;
  auto &data = GearIndicator::m_VehData.Get(pVeh);
  for (auto &ind : data.vecIndicatorData) {
    ind.iCurrent = static_cast<uint>(gear);
  }
}

bool ME_GetDashboardLEDState(CVehicle *pVeh, ME_LightID ledId) {
  if (!pVeh) return false;
  auto matType = static_cast<eMaterialType>(ledId);
  return ModelInfoMgr::IsMaterialAvailable(pVeh, matType);
}

void ME_SetDashboardLEDState(CVehicle *pVeh, ME_LightID ledId, bool state) {
  if (!pVeh) return;
  auto matType = static_cast<eMaterialType>(ledId);
  if (state) {
    ModelInfoMgr::EnableMaterial(pVeh, matType);
  }
}

bool ME_IsClockAvailable(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = DigitalClockFeature::m_VehData.Get(pVeh);
  return data.m_pRootFrame != nullptr || data.m_pDigitsRoot != nullptr;
}

bool ME_IsClock12HourFormat(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = DigitalClockFeature::m_VehData.Get(pVeh);
  return data.m_b12HourFormat;
}

void ME_SetClock12HourFormat(CVehicle *pVeh, bool is12Hour) {
  if (!pVeh) return;
  auto &data = DigitalClockFeature::m_VehData.Get(pVeh);
  data.m_b12HourFormat = is12Hour;
}

// Dirt FX API
float ME_GetDirtLevel(CVehicle *pVeh) {
  return pVeh ? pVeh->m_fDirtLevel : 0.0f;
}

void ME_SetDirtLevel(CVehicle *pVeh, float dirtLevel) {
  if (pVeh) pVeh->m_fDirtLevel = std::clamp(dirtLevel, 0.0f, 15.0f);
}

// Remap API
int ME_GetRemapIndex(CVehicle *pVeh) {
  return pVeh ? pVeh->GetRemapIndex() : -1;
}

void ME_SetRemapIndex(CVehicle *pVeh, int remapIndex) {
  if (pVeh) pVeh->SetRemap(remapIndex);
}

bool ME_HasRemaps(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return false;
  return Remap::HasRemaps(modelIndex);
}

int ME_GetRemapCount(CVehicle *pVeh) {
  if (!pVeh) return 0;
  return Remap::GetRemapCount(pVeh->m_nModelIndex);
}

int ME_GetRemapCountByModel(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return 0;
  return Remap::GetRemapCount(modelIndex);
}

// Spoilers & Convertible Roof API
bool ME_IsRoofOpen(CVehicle *pVeh) {
  if (!pVeh) return false;
  return ConvertibleRoof::IsRoofOpen(pVeh);
}

void ME_SetRoofOpen(CVehicle *pVeh, bool open) {
  if (!pVeh) return;
  auto &data = ConvertibleRoof::m_VehData.Get(pVeh);
  data.m_bRoofTargetExpanded = open;
}

bool ME_HasConvertibleRoof(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = ConvertibleRoof::m_VehData.Get(pVeh);
  return !data.m_Roofs.empty() || !data.m_Boots.empty();
}

int ME_GetRoofAnimPhase(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = ConvertibleRoof::m_VehData.Get(pVeh);
  return static_cast<int>(data.m_phase);
}

int ME_GetSpoilerCount(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = Spoiler::m_VehData.Get(pVeh);
  return static_cast<int>(data.m_Spoilers.size());
}

float ME_GetSpoilerAngle(CVehicle *pVeh, int spoilerIndex) {
  if (!pVeh) return 0.0f;
  auto &data = Spoiler::m_VehData.Get(pVeh);
  if (spoilerIndex >= 0 && spoilerIndex < static_cast<int>(data.m_Spoilers.size())) {
    return data.m_Spoilers[spoilerIndex].m_fCurrentRotation;
  }
  return 0.0f;
}

void ME_SetSpoilerAngle(CVehicle *pVeh, int spoilerIndex, float angle) {
  if (!pVeh) return;
  auto &data = Spoiler::m_VehData.Get(pVeh);
  if (spoilerIndex >= 0 && spoilerIndex < static_cast<int>(data.m_Spoilers.size())) {
    data.m_Spoilers[spoilerIndex].m_fCurrentRotation = angle;
  }
}

// Doors & Animation API
bool ME_HasSlideDoors(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = SlideDoor::m_VehData.Get(pVeh);
  return !data.leftFront.empty() || !data.rightFront.empty() || !data.leftRear.empty() || !data.rightRear.empty();
}

bool ME_HasRotateDoors(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = RotateDoor::m_VehData.Get(pVeh);
  return !data.leftFront.empty() || !data.rightFront.empty() || !data.leftRear.empty() || !data.rightRear.empty() || !data.boot.empty() || !data.bonnet.empty();
}

// Wheels, Hubs & Chain API
bool ME_HasExtraWheels(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = ExtraWheel::m_VehData.Get(pVeh);
  for (int i = 0; i < static_cast<int>(eWheelPos::COUNT); ++i) {
    if (!data.pExtras[i].empty()) return true;
  }
  return false;
}

int ME_GetExtraWheelCount(CVehicle *pVeh, int wheelPos) {
  if (!pVeh || wheelPos < 0 || wheelPos >= static_cast<int>(eWheelPos::COUNT)) return 0;
  auto &data = ExtraWheel::m_VehData.Get(pVeh);
  return static_cast<int>(data.pExtras[wheelPos].size());
}

bool ME_HasWheelHubs(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = WheelHub::m_VehData.Get(pVeh);
  return data.m_pWRF || data.m_pHRF || data.m_pWRM || data.m_pHRM || data.m_pWRR || data.m_pHRR ||
         data.m_pWLF || data.m_pHLF || data.m_pWLM || data.m_pHLM || data.m_pWLR || data.m_pHLR;
}

bool ME_HasAnimatedChain(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = ChainFeature::m_VehData.Get(pVeh);
  return data.m_pRootFrame != nullptr || !data.m_FrameList.empty();
}

int ME_GetChainFrameIndex(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = ChainFeature::m_VehData.Get(pVeh);
  return static_cast<int>(data.m_nCurChain);
}

int ME_GetChainFrameCount(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = ChainFeature::m_VehData.Get(pVeh);
  return static_cast<int>(data.m_FrameList.size());
}

// Rollback Bed API
bool ME_IsRollbackBedExpanded(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = RollbackBed::m_VehData.Get(pVeh);
  return data.bExpanded;
}

void ME_SetRollbackBedExpanded(CVehicle *pVeh, bool expanded) {
  if (!pVeh) return;
  auto &data = RollbackBed::m_VehData.Get(pVeh);
  data.bExpanded = expanded;
}

bool ME_HasRollbackBed(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = RollbackBed::m_VehData.Get(pVeh);
  return data.pBedFrame != nullptr;
}

float ME_GetRollbackBedTilt(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  auto &data = RollbackBed::m_VehData.Get(pVeh);
  return data.fBedCurRot;
}

int ME_GetRollbackBedPistonCount(CVehicle *pVeh) {
  if (!pVeh) return 0;
  auto &data = RollbackBed::m_VehData.Get(pVeh);
  return static_cast<int>(data.m_Pistons.size());
}

float ME_GetRollbackBedPistonMove(CVehicle *pVeh, int pistonIndex) {
  if (!pVeh) return 0.0f;
  auto &data = RollbackBed::m_VehData.Get(pVeh);
  if (pistonIndex >= 0 && pistonIndex < static_cast<int>(data.m_Pistons.size())) {
    return data.m_Pistons[pistonIndex].fCurMove;
  }
  return 0.0f;
}

// Spotlights API
bool ME_IsSpotlightActive(CVehicle *pVeh) {
  if (!pVeh) return false;
  return SpotLights::IsEnabled(pVeh);
}

void ME_SetSpotlightActive(CVehicle *pVeh, bool active) {
  if (!pVeh) return;
  auto &data = SpotLights::m_VehData.Get(pVeh);
  data.bEnabled = active;
}

bool ME_HasSpotlight(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = SpotLights::m_VehData.Get(pVeh);
  return data.pFrame != nullptr;
}

// Ped Colors API
int ME_GetPedVariationCount(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return 0;
  auto jsonData = DataMgr::Get(modelIndex);
  if (jsonData.contains("pedcols") && jsonData["pedcols"].contains("variations") && jsonData["pedcols"]["variations"].is_array()) {
    return static_cast<int>(jsonData["pedcols"]["variations"].size());
  }
  return 0;
}

bool ME_GetPedColors(CPed *pPed, ME_Color *col1, ME_Color *col2, ME_Color *col3, ME_Color *col4) {
  if (!pPed) return false;
  auto &data = PedColors::GetPedData().Get(pPed);
  if (!data.m_bUsingPedCols || data.m_Colors.size() < 4) return false;
  if (col1) *col1 = {data.m_Colors[0].r, data.m_Colors[0].g, data.m_Colors[0].b, data.m_Colors[0].a};
  if (col2) *col2 = {data.m_Colors[1].r, data.m_Colors[1].g, data.m_Colors[1].b, data.m_Colors[1].a};
  if (col3) *col3 = {data.m_Colors[2].r, data.m_Colors[2].g, data.m_Colors[2].b, data.m_Colors[2].a};
  if (col4) *col4 = {data.m_Colors[3].r, data.m_Colors[3].g, data.m_Colors[3].b, data.m_Colors[3].a};
  return true;
}

bool ME_SetPedColors(CPed *pPed, ME_Color col1, ME_Color col2, ME_Color col3, ME_Color col4) {
  if (!pPed) return false;
  auto &data = PedColors::GetPedData().Get(pPed);
  if (data.m_Colors.size() < 4) {
    data.m_Colors.resize(4);
  }
  data.m_Colors[0] = CRGBA(col1.r, col1.g, col1.b, col1.a);
  data.m_Colors[1] = CRGBA(col2.r, col2.g, col2.b, col2.a);
  data.m_Colors[2] = CRGBA(col3.r, col3.g, col3.b, col3.a);
  data.m_Colors[3] = CRGBA(col4.r, col4.g, col4.b, col4.a);
  data.m_bUsingPedCols = true;
  return true;
}

// Audio & Sounds API
bool ME_PlaySound(CVehicle *pVeh, const char *soundPath, float volume) {
  if (!pVeh || !soundPath) return false;
  AudioMgr::Play3DSound(soundPath, pVeh->GetPosition(), pVeh, volume);
  return true;
}

float ME_GetBrakePressure(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  auto &sData = SoundEffects::m_VehData.Get(pVeh);
  return sData.m_fBrakePressure;
}

bool ME_PlayFileSound(const char *soundPath, float volume) {
  if (!soundPath) return false;
  AudioMgr::PlayFileSound(soundPath, volume);
  return true;
}

void ME_PlaySwitchSound(CVehicle *pVeh) {
  AudioMgr::PlaySwitchSound(pVeh);
}

bool ME_GetSirenAudioPath(int modeIndex, char *outBuffer, int maxLen) {
  if (!outBuffer || maxLen <= 0) return false;
  std::string path = AudioMgr::GetSirenAudioPath(modeIndex);
  if (path.empty()) return false;
  SafeStrCopy(outBuffer, maxLen, path);
  return true;
}

// Visual & Render Configuration
float ME_GetMaterialAmbientMul() {
  return ModelInfoMgr::GetMaterialAmbientMul();
}

void ME_SetMaterialAmbientMul(float mul) {
  ModelInfoMgr::gfMaterialAmbientMul = std::max(0.0f, mul);
}

float ME_GetCoronaDistanceMul() {
  return RenderUtil::GetCoronaDistanceMul();
}

float ME_GetCoronaNearClip() {
  return RenderUtil::GetCoronaNearClip();
}

float ME_GetLightShadowDistance() {
  return RenderUtil::GetLightShadowDistance();
}

float ME_GetHighBeamPointLightMul() {
  return LightsConfig::Get().fHighBeamPointLightMul;
}

bool ME_IsAutoIndicatorsOnSteerEnabled() {
  return LightsConfig::Get().bAutoIndicatorsOnSteer;
}

bool ME_IsFoglightTiedToHeadlight() {
  return LightsConfig::Get().bFoglightTiedToHeadlight;
}

bool ME_IsPlayerIdleBrakeLightsEnabled() {
  return LightsConfig::Get().bPlayerIdleBrakeLights;
}

// Vehicle Physics & Auxiliary
float ME_GetVehiclePitch(CVehicle *pVeh) {
  if (!pVeh) return 0.0f;
  return CarUtil::GetVehiclePitch(pVeh);
}

bool ME_IsVehicleDoingWheelie(CVehicle *pVeh) {
  if (!pVeh) return false;
  return CarUtil::IsVehicleDoingWheelie(pVeh);
}

bool ME_IsVehicleEngineOff(CVehicle *pVeh) {
  if (!pVeh) return true;
  return CarUtil::IsEngineOff(pVeh);
}

// Helpers for JSON config extraction
static bool ParseJsonColor(const nlohmann::json &val, ME_Color &outCol, const nlohmann::json *pRoot = nullptr) {
  if (val.is_array() && val.size() >= 3) {
    outCol.r = val[0].get<uint8_t>();
    outCol.g = val[1].get<uint8_t>();
    outCol.b = val[2].get<uint8_t>();
    outCol.a = val.size() >= 4 ? val[3].get<uint8_t>() : 255;
    return true;
  }
  if (val.is_object()) {
    outCol.r = val.value("red", val.value("r", 255));
    outCol.g = val.value("green", val.value("g", 255));
    outCol.b = val.value("blue", val.value("b", 255));
    outCol.a = val.value("alpha", val.value("a", 255));
    return true;
  }
  if (val.is_string() && pRoot && pRoot->contains("colors") && (*pRoot)["colors"].contains(val.get<std::string>())) {
    return ParseJsonColor((*pRoot)["colors"][val.get<std::string>()], outCol, pRoot);
  }
  return false;
}

// ==========================================
// JSONC Configuration Export API Implementation
// ==========================================

// Model Data & Metadata
bool ME_HasModelData(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return false;
  return DataMgr::Has(modelIndex);
}

bool ME_GetModelDataPath(int modelIndex, char *outPath, int maxLen) {
  if (modelIndex < 0 || modelIndex >= 20000 || !outPath || maxLen <= 0) return false;
  if (!DataMgr::Has(modelIndex)) return false;
  const auto &path = DataMgr::GetPath(modelIndex);
  SafeStrCopy(outPath, maxLen, path);
  return true;
}

bool ME_GetModelMetadata(int modelIndex, char *outAuthor, int maxAuthorLen, char *outDesc, int maxDescLen, char *outCreationTime, int maxCreationTimeLen, int *outMinVer) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson) return false;
  const nlohmann::json *pMeta = nullptr;
  if (pJson->contains("metadata")) pMeta = &(*pJson)["metadata"];
  else if (pJson->contains("Metadata")) pMeta = &(*pJson)["Metadata"];
  if (!pMeta) return false;

  if (outAuthor && maxAuthorLen > 0) {
    SafeStrCopy(outAuthor, maxAuthorLen, pMeta->value("author", ""));
  }
  if (outDesc && maxDescLen > 0) {
    SafeStrCopy(outDesc, maxDescLen, pMeta->value("desc", ""));
  }
  if (outCreationTime && maxCreationTimeLen > 0) {
    SafeStrCopy(outCreationTime, maxCreationTimeLen, pMeta->value("creationtime", ""));
  }
  if (outMinVer) {
    *outMinVer = pMeta->value("minver", MOD_VERSION_NUMBER);
  }
  return true;
}

// Vehicle Lights JSON Config
bool ME_GetGlobalLightInertia(int modelIndex, float *outInertia) {
  if (!outInertia) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("lights")) return false;
  const auto &lights = (*pJson)["lights"];
  if (lights.contains("inertia")) {
    *outInertia = lights.value("inertia", 0.0f);
    return true;
  }
  return false;
}

bool ME_GetLightDummyConfig(int modelIndex, const char *dummyName, ME_Color *outCoronaColor, float *outCoronaSize, int *outLightingType, ME_Color *outShadowColor, float *outShadowSize, char *outShadowTexture, int maxTextureLen, bool *outRotationChecks, float *outInertia, int *outStrobeDelay) {
  if (!dummyName) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("lights")) return false;
  const auto &lights = (*pJson)["lights"];
  if (!lights.contains(dummyName)) return false;
  const auto &dSec = lights[dummyName];

  if (dSec.contains("corona")) {
    const auto &cSec = dSec["corona"];
    if (outCoronaColor && cSec.contains("color")) {
      ParseJsonColor(cSec["color"], *outCoronaColor, pJson);
    }
    if (outCoronaSize) {
      *outCoronaSize = cSec.value("size", 0.35f);
    }
    if (outLightingType) {
      *outLightingType = static_cast<int>(GetLightingMode(cSec.value("type", "directional")));
    }
  }
  if (dSec.contains("shadow")) {
    const auto &sSec = dSec["shadow"];
    if (outShadowColor && sSec.contains("color")) {
      ParseJsonColor(sSec["color"], *outShadowColor, pJson);
    }
    if (outShadowSize) {
      *outShadowSize = sSec.value("size", 1.0f);
    }
    if (outShadowTexture && maxTextureLen > 0) {
      SafeStrCopy(outShadowTexture, maxTextureLen, sSec.value("texture", "pointlight"));
    }
    if (outRotationChecks) {
      *outRotationChecks = sSec.value("rotationchecks", true);
    }
  }
  if (outInertia && dSec.contains("inertia")) {
    *outInertia = dSec.value("inertia", 0.0f);
  }
  if (outStrobeDelay && dSec.contains("strobedelay")) {
    *outStrobeDelay = dSec.value("strobedelay", 1000);
  }
  return true;
}

bool ME_GetLightGroupConfig(int modelIndex, ME_LightID lightId, ME_Color *outCoronaColor, float *outCoronaSize, int *outLightingType, ME_Color *outShadowColor, float *outShadowSize, char *outShadowTexture, int maxTextureLen, float *outInertia) {
  auto matType = static_cast<eMaterialType>(lightId);
  const char *grpKey = LightManager::GetLightGroupKey(matType);
  const char *specKey = LightManager::GetLightSpecificKey(matType);
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("lights")) return false;
  const auto &lights = (*pJson)["lights"];

  const nlohmann::json *pSec = nullptr;
  if (specKey && lights.contains(specKey)) pSec = &lights[specKey];
  else if (grpKey && lights.contains(grpKey)) pSec = &lights[grpKey];
  if (!pSec) return false;

  if (pSec->contains("corona")) {
    const auto &cSec = (*pSec)["corona"];
    if (outCoronaColor && cSec.contains("color")) {
      ParseJsonColor(cSec["color"], *outCoronaColor, pJson);
    }
    if (outCoronaSize) {
      *outCoronaSize = cSec.value("size", 0.35f);
    }
    if (outLightingType) {
      *outLightingType = static_cast<int>(GetLightingMode(cSec.value("type", "directional")));
    }
  }
  if (pSec->contains("shadow")) {
    const auto &sSec = (*pSec)["shadow"];
    if (outShadowColor && sSec.contains("color")) {
      ParseJsonColor(sSec["color"], *outShadowColor, pJson);
    }
    if (outShadowSize) {
      *outShadowSize = sSec.value("size", 1.0f);
    }
    if (outShadowTexture && maxTextureLen > 0) {
      SafeStrCopy(outShadowTexture, maxTextureLen, sSec.value("texture", "pointlight"));
    }
  }
  if (outInertia && pSec->contains("inertia")) {
    *outInertia = pSec->value("inertia", 0.0f);
  }
  return true;
}

// Sirens JSON Config
bool ME_IsSirenImVehFt(int modelIndex) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("sirens")) return false;
  return (*pJson)["sirens"].value("imvehft", false);
}

bool ME_GetSirenStateName(int modelIndex, int stateIndex, char *outName, int maxNameLen) {
  if (!outName || maxNameLen <= 0 || stateIndex < 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("sirens") || !(*pJson)["sirens"].contains("states")) return false;
  const auto &states = (*pJson)["sirens"]["states"];
  if (!states.is_object()) return false;
  int idx = 0;
  for (auto it = states.begin(); it != states.end(); ++it, ++idx) {
    if (idx == stateIndex) {
      SafeStrCopy(outName, maxNameLen, it.key());
      return true;
    }
  }
  return false;
}

int ME_GetSirenItemCount(int modelIndex, int stateIndex) {
  if (stateIndex < 0) return 0;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("sirens") || !(*pJson)["sirens"].contains("states")) return 0;
  const auto &states = (*pJson)["sirens"]["states"];
  if (!states.is_object()) return 0;
  int idx = 0;
  for (auto it = states.begin(); it != states.end(); ++it, ++idx) {
    if (idx == stateIndex && it.value().is_object()) {
      return static_cast<int>(it.value().size());
    }
  }
  return 0;
}

bool ME_GetSirenItemConfig(int modelIndex, int stateIndex, const char *sirenKey, ME_Color *outColor, float *outSize, float *outInertia, int *outStartState, char *outType, int maxTypeLen, ME_Color *outShadowColor, float *outShadowSize, char *outShadowType, int maxShadowTypeLen, float *outAngleOffset) {
  if (!sirenKey || stateIndex < 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("sirens") || !(*pJson)["sirens"].contains("states")) return false;
  const auto &states = (*pJson)["sirens"]["states"];
  if (!states.is_object()) return false;
  int idx = 0;
  const nlohmann::json *pStateSec = nullptr;
  for (auto it = states.begin(); it != states.end(); ++it, ++idx) {
    if (idx == stateIndex) {
      pStateSec = &it.value();
      break;
    }
  }
  if (!pStateSec || !pStateSec->contains(sirenKey)) return false;
  const auto &item = (*pStateSec)[sirenKey];

  if (outColor && item.contains("color")) {
    ParseJsonColor(item["color"], *outColor, pJson);
  }
  if (outSize) {
    *outSize = item.value("size", 0.35f);
  }
  if (outInertia) {
    *outInertia = item.value("inertia", 0.0f);
  }
  if (outStartState) {
    *outStartState = item.value("state", 1);
  }
  if (outType && maxTypeLen > 0) {
    SafeStrCopy(outType, maxTypeLen, item.value("type", "directional"));
  }
  if (item.contains("shadow")) {
    const auto &sh = item["shadow"];
    if (outShadowColor && sh.contains("color")) {
      ParseJsonColor(sh["color"], *outShadowColor, pJson);
    }
    if (outShadowSize) {
      *outShadowSize = sh.value("size", 1.0f);
    }
    if (outShadowType && maxShadowTypeLen > 0) {
      SafeStrCopy(outShadowType, maxShadowTypeLen, sh.value("type", "pointlight"));
    }
    if (outAngleOffset) {
      *outAngleOffset = sh.value("angleoffset", 0.0f);
    }
  }
  return true;
}

bool ME_GetSirenItemPattern(int modelIndex, int stateIndex, const char *sirenKey, int *outPatternDelays, int maxDelays, int *outDelayCount) {
  if (!sirenKey || stateIndex < 0 || !outPatternDelays || maxDelays <= 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("sirens") || !(*pJson)["sirens"].contains("states")) return false;
  const auto &states = (*pJson)["sirens"]["states"];
  if (!states.is_object()) return false;
  int idx = 0;
  const nlohmann::json *pStateSec = nullptr;
  for (auto it = states.begin(); it != states.end(); ++it, ++idx) {
    if (idx == stateIndex) {
      pStateSec = &it.value();
      break;
    }
  }
  if (!pStateSec || !pStateSec->contains(sirenKey)) return false;
  const auto &item = (*pStateSec)[sirenKey];
  if (!item.contains("pattern") || !item["pattern"].is_array()) return false;
  const auto &pat = item["pattern"];
  int count = std::min(static_cast<int>(pat.size()), maxDelays);
  for (int i = 0; i < count; ++i) {
    outPatternDelays[i] = pat[i].is_number() ? pat[i].get<int>() : 0;
  }
  if (outDelayCount) {
    *outDelayCount = static_cast<int>(pat.size());
  }
  return true;
}

// Gauges JSON Config
bool ME_GetSpeedometerConfig(int modelIndex, const char *nodeName, float *outMaxSpeed, float *outMaxRotation, bool *outIsKph) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("gauges")) return false;
  std::string name = nodeName ? nodeName : "x_sm";
  if (!(*pJson)["gauges"].contains(name)) return false;
  const auto &g = (*pJson)["gauges"][name];
  if (outMaxSpeed) *outMaxSpeed = g.value("maxspeed", 240.0f);
  if (outMaxRotation) *outMaxRotation = g.value("maxrotation", 240.0f);
  if (outIsKph) *outIsKph = g.value("kph", true);
  return true;
}

bool ME_GetRPMConfig(int modelIndex, const char *nodeName, float *outMaxRPM, float *outMaxRotation) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("gauges")) return false;
  std::string name = nodeName ? nodeName : "x_rpm";
  if (!(*pJson)["gauges"].contains(name)) return false;
  const auto &g = (*pJson)["gauges"][name];
  if (outMaxRPM) *outMaxRPM = g.value("maxrpm", 8000.0f);
  if (outMaxRotation) *outMaxRotation = g.value("maxrotation", 240.0f);
  return true;
}

bool ME_GetTurboConfig(int modelIndex, const char *nodeName, float *outMaxTurbo, float *outMaxRotation) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("gauges")) return false;
  std::string name = nodeName ? nodeName : "x_tm";
  if (!(*pJson)["gauges"].contains(name)) return false;
  const auto &g = (*pJson)["gauges"][name];
  if (outMaxTurbo) *outMaxTurbo = g.value("maxturbo", 2.0f);
  if (outMaxRotation) *outMaxRotation = g.value("maxrotation", 240.0f);
  return true;
}

bool ME_GetFixedGaugeConfig(int modelIndex, const char *nodeName, float *outMinAngle, float *outMaxAngle) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("gauges")) return false;
  std::string name = nodeName ? nodeName : "x_gauge_fixed";
  if (!(*pJson)["gauges"].contains(name)) return false;
  const auto &g = (*pJson)["gauges"][name];
  if (outMinAngle) *outMinAngle = g.value("minangle", 20.0f);
  if (outMaxAngle) *outMaxAngle = g.value("maxangle", 70.0f);
  return true;
}

bool ME_GetOdometerConfig(int modelIndex, const char *nodeName, bool *outIsKph) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("gauges")) return false;
  std::string name = nodeName ? nodeName : "x_odometer";
  if (!(*pJson)["gauges"].contains(name)) return false;
  const auto &g = (*pJson)["gauges"][name];
  if (outIsKph) *outIsKph = g.value("kph", true);
  return true;
}

// Clocks JSON Config
bool ME_GetClockConfig(int modelIndex, const char *nodeName, bool *out12HourFormat) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("clocks")) return false;
  std::string name = nodeName ? nodeName : "x_dclock";
  if (!(*pJson)["clocks"].contains(name)) return false;
  const auto &c = (*pJson)["clocks"][name];
  if (out12HourFormat) *out12HourFormat = c.value("12hformat", false);
  return true;
}

// Animated Doors JSON Config
bool ME_GetSlideDoorConfig(int modelIndex, const char *nodeName, float *outMovMul, float *outPopOut) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("doors") || !nodeName) return false;
  if (!(*pJson)["doors"].contains(nodeName)) return false;
  const auto &d = (*pJson)["doors"][nodeName];
  if (outMovMul) *outMovMul = d.value("movmul", 1.0f);
  if (outPopOut) *outPopOut = d.value("popout", 0.15f);
  return true;
}

bool ME_GetRotateDoorConfig(int modelIndex, const char *nodeName, float *outMul, float *outPopOut) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("doors") || !nodeName) return false;
  if (!(*pJson)["doors"].contains(nodeName)) return false;
  const auto &d = (*pJson)["doors"][nodeName];
  if (outMul) *outMul = d.value("mul", 1.0f);
  if (outPopOut) *outPopOut = d.value("popout", 0.15f);
  return true;
}

// Spoilers JSON Config
bool ME_GetSpoilerConfig(int modelIndex, const char *nodeName, float *outRotation, float *outTimeMs, float *outTriggerSpeed) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("spoilers")) return false;
  std::string name = nodeName ? nodeName : "movspoiler";
  if (!(*pJson)["spoilers"].contains(name)) return false;
  const auto &s = (*pJson)["spoilers"][name];
  if (outRotation) *outRotation = s.value("rotation", 30.0f);
  if (outTimeMs) *outTimeMs = s.value("time", 3000.0f);
  if (outTriggerSpeed) *outTriggerSpeed = s.value("triggerspeed", 20.0f);
  return true;
}

// Convertible Roofs JSON Config
bool ME_GetRoofConfig(int modelIndex, const char *nodeName, float *outRotation, float *outSpeed) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("roofs") || !nodeName) return false;
  if (!(*pJson)["roofs"].contains(nodeName)) return false;
  const auto &r = (*pJson)["roofs"][nodeName];
  if (outRotation) *outRotation = r.value("rotation", 60.0f);
  if (outSpeed) *outSpeed = r.value("speed", 2.0f);
  return true;
}

// Rollback Bed JSON Config
bool ME_GetRollbackBedConfig(int modelIndex, float *outBedTargetRot, float *outBedRotSpeed, float *outHyTargetRot, float *outHyRotSpeed, float *outHyTargetMove, float *outHyMoveSpeed) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("rollback_bed")) return false;
  const auto &rb = (*pJson)["rollback_bed"];
  if (rb.contains("bed")) {
    const auto &bed = rb["bed"];
    if (outBedTargetRot) *outBedTargetRot = bed.value("target_rot", 12.0f);
    if (outBedRotSpeed) *outBedRotSpeed = bed.value("rot_speed", 1.0f);
  }
  if (rb.contains("hydraulics")) {
    const auto &hy = rb["hydraulics"];
    if (outHyTargetRot) *outHyTargetRot = hy.value("target_rot", 18.0f);
    if (outHyRotSpeed) *outHyRotSpeed = hy.value("rot_speed", 1.0f);
    if (outHyTargetMove) *outHyTargetMove = hy.value("target_move", 2.0f);
    if (outHyMoveSpeed) *outHyMoveSpeed = hy.value("move_speed", 1.0f);
  }
  return true;
}

// Exhausts JSON Config
bool ME_GetExhaustConfig(int modelIndex, const char *nodeName, float *outLifetime, float *outSpeed, float *outSize, bool *outNitroEffect, ME_Color *outColor) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("exhausts") || !nodeName) return false;
  if (!(*pJson)["exhausts"].contains(nodeName)) return false;
  const auto &ex = (*pJson)["exhausts"][nodeName];
  if (outLifetime) *outLifetime = ex.value("lifetime", 0.4f);
  if (outSpeed) *outSpeed = ex.value("speed", 1.0f);
  if (outSize) *outSize = ex.value("size", 1.0f);
  if (outNitroEffect) *outNitroEffect = ex.value("nitro_effect", true);
  if (outColor && ex.contains("color")) {
    ParseJsonColor(ex["color"], *outColor, pJson);
  }
  return true;
}

// License Plate JSON Config
bool ME_GetPlateConfig(int modelIndex, ME_Color *outOnColor, ME_Color *outOffColor) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson) return false;
  const nlohmann::json *pSec = nullptr;
  if (pJson->contains("plate")) pSec = &(*pJson)["plate"];
  else if (pJson->contains("license_plate")) pSec = &(*pJson)["license_plate"];
  else if (pJson->contains("lights")) {
    if ((*pJson)["lights"].contains("plate")) pSec = &(*pJson)["lights"]["plate"];
    else if ((*pJson)["lights"].contains("license_plate")) pSec = &(*pJson)["lights"]["license_plate"];
  }
  if (!pSec) return false;

  bool foundOn = false;
  bool foundOff = false;
  if (outOnColor) {
    if (pSec->contains("color")) foundOn = ParseJsonColor((*pSec)["color"], *outOnColor, pJson);
    else if (pSec->contains("material") && (*pSec)["material"].contains("color")) foundOn = ParseJsonColor((*pSec)["material"]["color"], *outOnColor, pJson);
  }
  if (outOffColor) {
    if (pSec->contains("color_off")) foundOff = ParseJsonColor((*pSec)["color_off"], *outOffColor, pJson);
    else if (pSec->contains("material") && (*pSec)["material"].contains("color_off")) foundOff = ParseJsonColor((*pSec)["material"]["color_off"], *outOffColor, pJson);
  }
  return foundOn || foundOff;
}

// Carcols JSON Config
int ME_GetCarcolColorCount(int modelIndex) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("carcols") || !(*pJson)["carcols"].contains("colors") || !(*pJson)["carcols"]["colors"].is_array()) return 0;
  return static_cast<int>((*pJson)["carcols"]["colors"].size());
}

bool ME_GetCarcolColor(int modelIndex, int colorIndex, ME_Color *outColor) {
  if (!outColor || colorIndex < 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("carcols") || !(*pJson)["carcols"].contains("colors") || !(*pJson)["carcols"]["colors"].is_array()) return false;
  const auto &cols = (*pJson)["carcols"]["colors"];
  if (colorIndex >= static_cast<int>(cols.size())) return false;
  return ParseJsonColor(cols[colorIndex], *outColor, pJson);
}

bool ME_GetCarcolVariationData(int modelIndex, int varIndex, int *outPrimary, int *outSecondary, int *outTertiary, int *outQuaternary) {
  if (varIndex < 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("carcols") || !(*pJson)["carcols"].contains("variations") || !(*pJson)["carcols"]["variations"].is_array()) return false;
  const auto &vars = (*pJson)["carcols"]["variations"];
  if (varIndex >= static_cast<int>(vars.size())) return false;
  const auto &v = vars[varIndex];
  if (outPrimary) *outPrimary = v.value("primary", 0);
  if (outSecondary) *outSecondary = v.value("secondary", 0);
  if (outTertiary) *outTertiary = v.value("tertiary", 0);
  if (outQuaternary) *outQuaternary = v.value("quaternary", 0);
  return true;
}

// PedCols JSON Config
int ME_GetPedColColorCount(int modelIndex) {
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("pedcols") || !(*pJson)["pedcols"].contains("colors") || !(*pJson)["pedcols"]["colors"].is_array()) return 0;
  return static_cast<int>((*pJson)["pedcols"]["colors"].size());
}

bool ME_GetPedColColor(int modelIndex, int colorIndex, ME_Color *outColor) {
  if (!outColor || colorIndex < 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("pedcols") || !(*pJson)["pedcols"].contains("colors") || !(*pJson)["pedcols"]["colors"].is_array()) return false;
  const auto &cols = (*pJson)["pedcols"]["colors"];
  if (colorIndex >= static_cast<int>(cols.size())) return false;
  return ParseJsonColor(cols[colorIndex], *outColor, pJson);
}

bool ME_GetPedColVariationData(int modelIndex, int varIndex, int *outPrimary, int *outSecondary, int *outTertiary, int *outQuaternary) {
  if (varIndex < 0) return false;
  const auto *pJson = DataMgr::Find(modelIndex);
  if (!pJson || !pJson->contains("pedcols") || !(*pJson)["pedcols"].contains("variations") || !(*pJson)["pedcols"]["variations"].is_array()) return false;
  const auto &vars = (*pJson)["pedcols"]["variations"];
  if (varIndex >= static_cast<int>(vars.size())) return false;
  const auto &v = vars[varIndex];
  if (outPrimary) *outPrimary = v.value("primary", 0);
  if (outSecondary) *outSecondary = v.value("secondary", 0);
  if (outTertiary) *outTertiary = v.value("tertiary", 0);
  if (outQuaternary) *outQuaternary = v.value("quaternary", 0);
  return true;
}

} // extern "C"

