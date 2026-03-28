#include "pch.h"
#include "core/base.h"
#include "loader.h"
#include "db/ini.hpp"
#include <format>

extern CIniReader gConfig;

CBaseFeature::CBaseFeature(std::string name, std::string configSection, eFeatureMatrix featureId)
    : m_name(std::move(name)), m_configSection(std::move(configSection)), m_featureId(featureId) 
{
    if (IsActive()) {
        m_bActive = true;
        ModelExtras::m_Features.push_back(this);
        if (m_featureId <= eFeatureMatrix::FeatureCount) {
            ModelExtras::m_bEnabledFeatures.set(static_cast<int>(m_featureId));
        }
        LOG(INFO) << std::format("Feature '{}' enabled.", m_name);
    }
}

bool CBaseFeature::IsActive() const {
    return gConfig.ReadBoolean(m_configSection, m_name, false);
}
