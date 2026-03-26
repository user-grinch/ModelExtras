#pragma once
#include "pch.h"
#include <bitset>
#include "enums/featurematrix.h"
#include <vector>
#include "features/core/base.h"

class ModelExtrasLoader {
public:
    static inline std::vector<CBaseFeature*> m_Features;
    static inline std::bitset<static_cast<int>(eFeatureMatrix::FeatureCount)> m_bEnabledFeatures;

    static void Init();
    static void Reload(CVehicle *pVeh);
};