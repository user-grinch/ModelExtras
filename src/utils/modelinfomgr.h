#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

#include <plugin.h>
#include <game_sa/CVehicle.h>
#include <game_sa/CPools.h>
#include <game_sa/CModelInfo.h>
#include <RenderWare.h>
#include "enums/dummypos.h"
#include "enums/materialtype.h"

struct MatStateColor {
  CRGBA on, off;
};

constexpr uint32_t MAX_LIGHTS = 256;
inline const CRGBA DEFAULT_MAT_COL = CRGBA(255, 255, 255, 255);

using DummyCallback_t =
    std::function<void(CVehicle *, RwFrame *, const std::string_view &)>;
using RenderCallback_t = std::function<void(CVehicle *)>;
using MaterialCallback_t =
    std::function<eMaterialType(CVehicle *, RpMaterial *)>;
using MaterialColProviderCallback_t =
    std::function<MatStateColor(CVehicle *, RpMaterial *, eMaterialType)>;

struct VehModelData {
  std::array<bool, eMaterialType::TotalMaterial> m_MatStatus{};
  std::array<bool, eMaterialType::TotalMaterial> m_MatAvail{};
  std::array<bool, MAX_LIGHTS> m_SirenStatus{};
  std::array<bool, MAX_LIGHTS> m_StrobeStatus{};
  uint32_t nFrameCount = 0;

  VehModelData() = default;
  explicit VehModelData(CVehicle *) {}
};

class ModelInfoMgr {
private:
  static inline std::vector<DummyCallback_t> dummies;
  static inline std::vector<MaterialCallback_t> materials;
  static inline std::vector<MaterialColProviderCallback_t> matColProviders;
  static inline std::vector<RenderCallback_t> renders;

  static inline plugin::VehicleExtendedData<VehModelData> m_VehData;

  static void FindDummies(CVehicle *vehicle, RwFrame *frame);
  static void OnRender(CVehicle *pVeh);
  static MatStateColor FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat,
                                        eMaterialType type);
  static eMaterialType FetchMaterialType(CVehicle *pVeh, RpMaterial *pMat);

  static RpMaterial *SetEditableMaterialsCB(RpMaterial *material, void *data);
  static void __cdecl ResetEditableMaterials();
  static void __fastcall SetupRender(CVehicle *ptr);

public:
  static void EnableMaterial(CVehicle *pVeh, eMaterialType type);
  static void EnableSirenMaterial(CVehicle *pVeh, int idx);
  static void EnableStrobeMaterial(CVehicle *pVeh, int idx);
  static bool IsMaterialAvailable(CVehicle *pVeh, eMaterialType type);

  static void Init();
  static void RegisterDummy(const DummyCallback_t &function);
  static void RegisterMaterial(const MaterialCallback_t &material);
  static void
  RegisterMaterialColProvider(const MaterialColProviderCallback_t &material);
  static void RegisterRender(const RenderCallback_t &render);
  static void Reload(CVehicle *pVeh);
};
