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
  virtual bool IsMatAvail(CVehicle *pVeh) final;

  virtual bool IsDummyAvail(CVehicle *pVeh, eMaterialType state) final;
  virtual void EnableDummy(CVehicle *pVeh, VehicleDummy *pDummy) final;

public:
  static inline std::vector<ILightBehaviorBase *> ptrs;

  ILightBehaviorBase() { ptrs.push_back(this); }

  virtual ~ILightBehaviorBase() {
    ptrs.erase(std::remove(ptrs.begin(), ptrs.end(), this), ptrs.end());
  }

  virtual bool IsValidDummy(RwFrame *pFrame) = 0;
  virtual bool IsValidMaterial(RpMaterial *pMat) = 0;
  virtual bool RegisterDummy(CVehicle *pVeh, RwFrame *pFrame) = 0;
  virtual bool RegisterMat(CVehicle *pVeh, RpMaterial *pMat) = 0;

  virtual std::vector<eMaterialType> GetSupportedMatTypes() = 0;
  virtual eMaterialType GetMatType(RpMaterial *pMat) = 0;

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
  
  virtual void RenderLight(CVehicle *pVeh, eMaterialType state);
  virtual void RenderLights(CVehicle *pControlVeh, CVehicle *pTowedVeh);
};
