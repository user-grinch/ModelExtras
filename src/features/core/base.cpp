#include "pch.h"
#include "core/base.h"
#include "loader.h"
#include "ini/ini.hpp"
#include <format>

extern CIniReader gConfig;

CBaseFeature::CBaseFeature(std::string name, std::string configSection, eFeatureMatrix featureId)
    : m_name(std::move(name)), m_configSection(std::move(configSection)), m_featureId(featureId) 
{
    m_bActive = IsActive();
    if (m_featureId <= eFeatureMatrix::FeatureCount) {
        ModelExtras::m_bEnabledFeatures.set(static_cast<int>(m_featureId), m_bActive);
    }
    if (m_bActive) {
        LOG(INFO) << std::format("Feature '{}' enabled.", m_name);
    }
}

bool CBaseFeature::IsActive() const {
    if (gConfig.ReadBoolean(m_configSection, m_name, false)) return true;
    if (gConfig.ReadBoolean("LIGHTS", m_name, false)) return true;
    if (gConfig.ReadBoolean("SOUND", m_name, false)) return true;
    if (gConfig.ReadBoolean("FEATURES", m_name, false)) return true;
    return false;
}

bool CBaseFeature::IsEnabled(eFeatureMatrix featureId) {
    if (static_cast<size_t>(featureId) < ModelExtras::m_bEnabledFeatures.size()) {
        return ModelExtras::m_bEnabledFeatures.test(static_cast<size_t>(featureId));
    }
    return false;
}

void CBaseFeature::ReloadConfig() {
    m_bActive = IsActive();
    if (m_featureId <= eFeatureMatrix::FeatureCount) {
        ModelExtras::m_bEnabledFeatures.set(static_cast<int>(m_featureId), m_bActive);
    }
}
