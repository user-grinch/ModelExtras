#pragma once
#include "pch.h"
#include <bitset>
#include "enums/featurematrix.h"
#include <vector>
#include <memory>
#include "features/core/base.h"

class ModelExtras {
public:
    static inline std::vector<std::unique_ptr<CBaseFeature>> m_Features;
    static inline std::bitset<static_cast<int>(eFeatureMatrix::FeatureCount)> m_bEnabledFeatures;

    template <typename T, typename... Args>
    static T* RegisterFeature(Args&&... args) {
        auto feat = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = feat.get();
        m_Features.push_back(std::move(feat));
        return ptr;
    }

    static void Init();
    static void Reload(CVehicle *pVeh);
};