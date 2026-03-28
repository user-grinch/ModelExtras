#include "ModelExtrasAPI.h"
#include "defines.h"
#include "loader.h"
#include "pch.h"

extern "C" {
int ME_GetAPIVersion() { return ME_API_VERSION; }

int ME_GetVersion() { return MOD_VERSION_NUMBER; }

bool ME_IsFeatureAvail(ME_FeatureID featureId) {
  auto idx = static_cast<int>(featureId);
  if (idx < 0 || idx >= static_cast<int>(eFeatureMatrix::FeatureCount)) {
    return false;
  }
  return ModelExtras::m_bEnabledFeatures.test(idx);
}

// Dummy function to show on crash logs
int __declspec(dllexport) ignore1(int i) { return 1; }
}
