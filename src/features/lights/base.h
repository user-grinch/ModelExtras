#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "CVehicle.h"
#include "config.h"
#include "core/dummyconfig.h"
#include "enums/materialtype.h"
#include <shared/extender/VehicleExtender.h>

class ILightBehaviorBase {
protected:
  VehicleExtendedData<LightsCommonData> commonData;

  virtual bool IsDummyAvail(CVehicle *pVeh) final;
  virtual bool IsDummyAvail(CVehicle *pVeh, eMaterialType state) final;
  virtual bool IsMatAvail(CVehicle *pVeh) final;
  virtual void EnableDummy(CVehicle *pVeh, VehicleDummy *pDummy) final;

  void RenderLight(CVehicle *pVeh, eMaterialType state);

  virtual void RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh);

public:
  static inline std::vector<ILightBehaviorBase *> ptrs;

  ILightBehaviorBase() { ptrs.push_back(this); }

  virtual ~ILightBehaviorBase() {
    ptrs.erase(std::remove(ptrs.begin(), ptrs.end(), this), ptrs.end());
  }

  virtual std::vector<eMaterialType> GetTypes() const {
    return {};
  }

  virtual bool IsValidDummy(RwFrame *frame) = 0;
  virtual bool IsValidDummy(const std::string& dummyName) = 0;
  virtual bool IsValidMaterial(RpMaterial *pMat) = 0;
  virtual bool IsValidMaterial(const CRGBA &color) = 0;
  virtual bool RegisterDummy(RwFrame *frame) = 0;
  virtual bool RegisterDummy(const std::string& dummyName) = 0;

  virtual VehicleDummyConfig GetDummyConfig(RwFrame *frame) = 0;
  virtual eMaterialType GetMatType(RpMaterial *pMat) = 0;
  virtual eMaterialType GetMatType(const CRGBA &color) = 0;

  virtual void Process(CVehicle *pVeh) = 0;
  virtual void Render(CVehicle *pControlVeh, CVehicle *pTowedVeh) = 0;

  VehicleExtendedData<LightsCommonData> &GetCommonData() {
    return commonData;
  }
};

template <typename T> class ILightBehavior : public ILightBehaviorBase {
protected:
  VehicleExtendedData<T> typeData;

public:
  ILightBehavior() = default;
  virtual ~ILightBehavior() = default;

  VehicleExtendedData<T> &GetTypeData() { return typeData; }
};
