#pragma once
#include "db/ini.hpp"
#include "enums/featurematrix.h"
#include "utils/util.h"
#include <algorithm>
#include <string>
#include <vector>

class CVehicle;

class CBaseFeature {
  friend class ModelExtrasLoader;

protected:
  std::string m_name;
  std::string m_configSection;
  eFeatureMatrix m_featureId;
  bool m_bActive = false;


public:
  CBaseFeature(std::string name, std::string configSection,
               eFeatureMatrix featureId);

  virtual ~CBaseFeature() = default;

  [[nodiscard]] bool IsActive() const;

  virtual void Init() = 0;
  virtual void Shutdown() {}
  virtual void Reload() {}
  virtual void Reload(CVehicle *pVeh) {}
};

template <typename T> class CVehFeature : public CBaseFeature {
public:
  static inline VehicleExtendedData<T> m_VehData;

  CVehFeature(std::string name, std::string configSection,
              eFeatureMatrix featureId)
      : CBaseFeature(std::move(name), std::move(configSection), featureId) {}

  virtual ~CVehFeature() = default;

  T &GetVehData(CVehicle *pVeh) { return m_VehData.Get(pVeh); }

  VehicleExtendedData<T> &GetVehExtendedData() { return m_VehData; }
};
