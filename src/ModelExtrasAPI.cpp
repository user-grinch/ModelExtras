#include "pch.h"
#include "ModelExtrasAPI.h"
#include "defines.h"
#include "loader.h"
#include "utils/datamgr.h"
#include "utils/modelinfomgr.h"
#include "utils/car.h"
#include "features/sirens.h"
#include "features/lights/data.h"
#include "features/lights/manager.h"
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
#include <algorithm>
#include <cstring>

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

float ME_GetSirenPointLightDistanceMul() {
  return Sirens::GetPointLightDistanceMul();
}

void ME_SetSirenPointLightDistanceMul(float mul) {
  Sirens::SetPointLightDistanceMul(mul);
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

void ME_SetExhaustData(CVehicle *pVeh, int index, ME_ExhaustInfo &info) {
  if (!pVeh) return;
  auto &data = ExhaustFx::m_VehData.Get(pVeh);
  if (index >= 0 && index < static_cast<int>(data.m_pDummies.size())) {
    auto &ex = data.m_pDummies[index].second;
    ex.pFrame = info.pFrame;
    ex.Color = {info.Color.r, info.Color.g, info.Color.b, info.Color.a};
    ex.fSpeedMul = info.fSpeedMul;
    ex.fLifeTime = info.fLifeTime;
    ex.fSizeMul = info.fSizeMul;
    ex.bNitroEffect = info.bNitroEffect;
  }
}

void ME_TriggerNitro(CVehicle *pVeh, bool enable) {
  if (!pVeh) return;
  if (pVeh->m_nVehicleSubClass == VEHICLE_AUTOMOBILE) {
    auto *pAuto = reinterpret_cast<CAutomobile *>(pVeh);
    pAuto->m_fNitroValue = enable ? -1.0f : 0.0f;
  }
}

// Backfire API
void ME_TriggerBackfire(CVehicle *pVeh, bool bPlaySound) {
  if (!pVeh) return;
  BackFireEffect::BackFireSingle(pVeh, bPlaySound);
}

bool ME_IsBackfireActive(CVehicle *pVeh) {
  if (!pVeh) return false;
  auto &data = BackFireEffect::m_VehData.Get(pVeh);
  return data.m_nleftFires > 0;
}

// Dashboard & Gauges API
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

float ME_GetSpoilerAngle(CVehicle *pVeh, int spoilerIndex) {
  if (!pVeh) return 0.0f;
  auto &data = Spoiler::m_VehData.Get(pVeh);
  if (spoilerIndex >= 0 && spoilerIndex < static_cast<int>(data.m_Spoilers.size())) {
    return data.m_Spoilers[spoilerIndex].m_fCurrentRotation;
  }
  return 0.0f;
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

// Ped Colors API
int ME_GetPedVariationCount(int modelIndex) {
  if (modelIndex < 0 || modelIndex >= 20000) return 0;
  auto jsonData = DataMgr::Get(modelIndex);
  if (jsonData.contains("pedcols") && jsonData["pedcols"].contains("variations") && jsonData["pedcols"]["variations"].is_array()) {
    return static_cast<int>(jsonData["pedcols"]["variations"].size());
  }
  return 0;
}

} // extern "C"

