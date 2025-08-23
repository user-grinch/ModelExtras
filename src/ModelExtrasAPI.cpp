#include "pch.h"
#include "defines.h"
#include "ModelExtrasAPI.h"
#include "mgr.h"

extern "C" {
    int ME_GetAPIVersion() {
        return ME_API_VERSION;
    }

    int ME_GetVersion() {
        return MOD_VERSION_NUMBER;
    }

    bool ME_IsFeatureAvail(ME_FeatureID featureId) {
        return FeatureMgr::m_bEnabledFeatures.test(featureId);
    }
}