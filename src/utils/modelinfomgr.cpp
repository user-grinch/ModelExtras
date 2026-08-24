#include "modelinfomgr.h"
#include "pch.h"

#include <CCamera.h>
#include <CTxdStore.h>
#include <NodeName.h>
#include <RenderWare.h>
#include <rpworld.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <string_view>
#include <winuser.h>

#include "features/carcols.h"
#include "features/dirtfx.h"
#include "features/plate.h"
#include "utils/meevents.h"
#include "utils/texmgr.h"

using namespace plugin;

extern int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
extern int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat);

static CVehicle *pCurVeh = nullptr;
RwSurfaceProperties &gLightSurfProps =
    *reinterpret_cast<RwSurfaceProperties *>(0x8A645C);
RwSurfaceProperties gLightSurfPropsOff = {0.45f, 0.0f, 0.0f};

static constexpr uint32_t RwFrameForAllObjectsAddr = 0x7F1200;
static constexpr uint32_t RwFrameAddChildAddr = 0x7F0B00;
static constexpr uint32_t GetCurrentAtomicObjectCBAddr = 0x6D33B0;

static void LogSkippedUpgradePart() {
  static int count = 0;
  if (count < 10) {
    ++count;
    LOG_VERBOSE("Skipped an upgrade part, the vehicle model has no frame for "
                "that slot");
    if (count == 10) {
      LOG_VERBOSE("Silencing further upgrade slot messages");
    }
  }
}

static RwFrame *UpgradeFrameForAllObjects(RwFrame *frame,
                                          RwObjectCallBack callback,
                                          void *data) {
  if (!frame) {
    if (data && callback == reinterpret_cast<RwObjectCallBack>(
                                GetCurrentAtomicObjectCBAddr)) {
      *reinterpret_cast<void **>(data) = nullptr;
    }
    LogSkippedUpgradePart();
    return frame;
  }
  return RwFrameForAllObjects(frame, callback, data);
}

static RwFrame *UpgradeFrameAddChild(RwFrame *parent, RwFrame *child) {
  if (!parent) {
    LogSkippedUpgradePart();
    return parent;
  }
  return RwFrameAddChild(parent, child);
}

static size_t GuardUpgradeFrameCalls(uint32_t start, uint32_t end) {
  size_t patched = 0;
  for (uint32_t addr = start; addr < end; ++addr) {
    if (*reinterpret_cast<uint8_t *>(addr) != 0xE8) {
      continue;
    }

    uint32_t target = addr + 5 + *reinterpret_cast<int32_t *>(addr + 1);
    if (target == RwFrameForAllObjectsAddr) {
      patch::ReplaceFunctionCall(
          addr, reinterpret_cast<void *>(UpgradeFrameForAllObjects));
      ++patched;
    } else if (target == RwFrameAddChildAddr) {
      patch::ReplaceFunctionCall(
          addr, reinterpret_cast<void *>(UpgradeFrameAddChild));
      ++patched;
    }
  }
  return patched;
}

void ModelInfoMgr::Init() {
  patch::Nop(0x4C8E53, 5);
  patch::Nop(0x4C8F6E, 5);

  size_t guarded = GuardUpgradeFrameCalls(0x6D3300, 0x6D3C00);
  guarded += GuardUpgradeFrameCalls(0x6DF900, 0x6DFC00);
  if (guarded > 0) {
    LOG_VERBOSE("Guarded {} vehicle upgrade frame calls", guarded);
  } else {
    LOG(ERROR) << "Found no vehicle upgrade frame calls to guard, the "
                  "addresses may have moved";
  }

  patch::ReplaceFunctionCall(
      0x5532A9, reinterpret_cast<void *>(ModelInfoMgr::SetupRender));
  patch::ReplaceFunction(
      0x4C8220, reinterpret_cast<void *>(ModelInfoMgr::SetEditableMaterialsCB));

  Events::initScriptsEvent += []() {
    gLightSurfProps.ambient = gConfig.ReadFloat("VISUAL", "MaterialAmbientOn",
                                                gLightSurfProps.ambient);
    gLightSurfPropsOff.ambient = gConfig.ReadFloat(
        "VISUAL", "MaterialAmbientOff", gLightSurfPropsOff.ambient);
  };

  MEEvents::vehRenderEvent.before += [](CVehicle *pVeh) {
    if (!pVeh || !pVeh->m_pRwClump) {
      return;
    }

    auto &data = m_VehData.Get(pVeh);
    if (data.nFrameCount > 10) {
      ModelInfoMgr::OnRender(pVeh);
    } else if (data.nFrameCount == 10) {
      ModelInfoMgr::FindDummies(
          pVeh, reinterpret_cast<RwFrame *>(pVeh->m_pRwClump->object.parent));
      data.nFrameCount++;
    } else {
      data.nFrameCount++;
    }
  };

  MEEvents::heliRenderEvent.after += [](CVehicle *pVeh) {
    if (pVeh && CModelInfo::IsHeliModel(pVeh->m_nModelIndex)) {
      ModelInfoMgr::OnRender(pVeh);
    }
  };
}

void ModelInfoMgr::RegisterRender(const RenderCallback_t &render) {
  renders.push_back(render);
}

void ModelInfoMgr::RegisterDummy(const DummyCallback_t &function) {
  dummies.push_back(function);
}

void ModelInfoMgr::EnableMaterial(CVehicle *pVeh, eMaterialType type) {
  if (type >= 0 && type < eMaterialType::TotalMaterial) {
    auto &data = m_VehData.Get(pVeh);
    data.m_MatStatus[type] = true;
  }
}

void ModelInfoMgr::EnableSirenMaterial(CVehicle *pVeh, int idx) {
  if (idx >= 0 && idx < static_cast<int>(MAX_LIGHTS)) {
    auto &data = m_VehData.Get(pVeh);
    data.m_SirenStatus[idx] = true;
  }
}

void ModelInfoMgr::EnableStrobeMaterial(CVehicle *pVeh, int idx) {
  if (idx >= 0 && idx < static_cast<int>(MAX_LIGHTS)) {
    auto &data = m_VehData.Get(pVeh);
    data.m_StrobeStatus[idx] = true;
  }
}

void ModelInfoMgr::FindDummies(CVehicle *vehicle, RwFrame *frame) {
  if (!frame) {
    return;
  }

  if (RwFrame *nextFrame = frame->child) {
    FindDummies(vehicle, nextFrame);
  }

  if (RwFrame *nextFrame = frame->next) {
    FindDummies(vehicle, nextFrame);
  }

  if (!dummies.empty()) {
    std::string_view nodeName = GetFrameNodeName(frame);
    for (const auto &e : dummies) {
      e(vehicle, frame, nodeName);
    }
  }
}

void ModelInfoMgr::Reload(CVehicle *pVeh) {
  if (pVeh && pVeh->m_pRwClump) {
    RwFrame *frame =
        reinterpret_cast<RwFrame *>(pVeh->m_pRwClump->object.parent);
    FindDummies(pVeh, frame);
  }
}

void ModelInfoMgr::OnRender(CVehicle *vehicle) {
  for (const auto &e : renders) {
    e(vehicle);
  }
}

void ModelInfoMgr::RegisterMaterial(const MaterialCallback_t &mat) {
  materials.push_back(mat);
}

void ModelInfoMgr::RegisterMaterialColProvider(
    const MaterialColProviderCallback_t &mat) {
  matColProviders.push_back(mat);
}

void ModelInfoMgr::SetupRender(CVehicle *ptr) {
  pCurVeh = ptr;
  auto &data = m_VehData.Get(pCurVeh);
  ptr->SetupRender();

  data.m_MatStatus.fill(false);
  data.m_SirenStatus.fill(false);
  data.m_StrobeStatus.fill(false);
}

struct tRestoreEntry {
  void *m_pAddress;
  void *m_pValue;
};

static tRestoreEntry *const gRestoreEntries =
    reinterpret_cast<tRestoreEntry *>(0xB4DBE8);
static constexpr ptrdiff_t RESTORE_ENTRY_COUNT = 256;
static constexpr ptrdiff_t RESTORE_ENTRY_RANGE = RESTORE_ENTRY_COUNT * 2;

static bool CanStoreRestoreEntries(tRestoreEntry **ppEntries,
                                   ptrdiff_t needed) {
  tRestoreEntry *pEntry = *ppEntries;
  if (pEntry < gRestoreEntries ||
      pEntry >= gRestoreEntries + RESTORE_ENTRY_RANGE) {
    return true;
  }

  return (pEntry - gRestoreEntries) + needed + 1 <= RESTORE_ENTRY_COUNT;
}

static void LogRestoreEntriesFull() {
  static int count = 0;
  if (count < 10) {
    ++count;
    LOG_VERBOSE("Ran out of material restore entries, this model has too many "
                "editable materials");
    if (count == 10) {
      LOG_VERBOSE("Silencing further material restore entry messages");
    }
  }
}

MatStateColor ModelInfoMgr::FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat,
                                             eMaterialType type) {
  MatStateColor col = {DEFAULT_MAT_COL, DEFAULT_MAT_COL};
  for (const auto &e : matColProviders) {
    col = e(pVeh, pMat, type);
    if (col.on != DEFAULT_MAT_COL || col.off != DEFAULT_MAT_COL) {
      break;
    }
  }
  return col;
}

eMaterialType ModelInfoMgr::FetchMaterialType(CVehicle *pVeh,
                                              RpMaterial *pMat) {
  for (const auto &e : materials) {
    eMaterialType type = e(pVeh, pMat);
    if (type != eMaterialType::UnknownMaterial) {
      return type;
    }
  }
  return eMaterialType::UnknownMaterial;
}

RpMaterial *ModelInfoMgr::SetEditableMaterialsCB(RpMaterial *material,
                                                 void *data) {
  if (!material) {
    return material;
  }

  tRestoreEntry **ppEntries = reinterpret_cast<tRestoreEntry **>(data);
  if (material->texture) {
    const char *texName = material->texture->name;
    bool isRemapTex = (texName && texName[0] == '#');
    if (isRemapTex) {
      if (CVehicleModelInfo::ms_pRemapTexture &&
          CanStoreRestoreEntries(ppEntries, 1)) {
        (*ppEntries)->m_pAddress = &material->texture;
        (*ppEntries)->m_pValue = material->texture;
        (*ppEntries)++;
        material->texture = CVehicleModelInfo::ms_pRemapTexture;
      }
    } else if (pCurVeh) {
      DirtFx::ProcessTextures(pCurVeh, material);
      LicensePlate::ProcessTextures(pCurVeh, material);
    }
  }

  if (!pCurVeh) {
    return material;
  }

  eMaterialType iLightIndex = FetchMaterialType(pCurVeh, material);

  if (iLightIndex != eMaterialType::UnknownMaterial && iLightIndex >= 0 &&
      iLightIndex < eMaterialType::TotalMaterial) {
    auto &vData = m_VehData.Get(pCurVeh);

    bool lightOn = false;
    vData.m_MatAvail[iLightIndex] = true;

    if (iLightIndex == eMaterialType::SirenLight) {
      int idx = GetSirenIndex(pCurVeh, material);
      if (idx >= 0 && idx < static_cast<int>(MAX_LIGHTS)) {
        lightOn = vData.m_SirenStatus[idx];
      }
    } else if (iLightIndex == eMaterialType::StrobeLight) {
      int idx = GetStrobeIndex(pCurVeh, material);
      if (idx >= 0 && idx < static_cast<int>(MAX_LIGHTS)) {
        lightOn = vData.m_StrobeStatus[idx];
      }
    } else {
      lightOn = vData.m_MatStatus[iLightIndex];
    }

    MatStateColor matCol = FetchMaterialCol(pCurVeh, material, iLightIndex);
    if (!CanStoreRestoreEntries(ppEntries, lightOn ? 2 : 1)) {
      LogRestoreEntriesFull();
      return material;
    }

    RwRGBA *pColor = RpMaterialGetColor(material);
    (*ppEntries)->m_pAddress = pColor;
    (*ppEntries)->m_pValue = *reinterpret_cast<void **>(pColor);
    (*ppEntries)++;

    pColor->red = matCol.on.r;
    pColor->green = matCol.on.g;
    pColor->blue = matCol.on.b;

    if (lightOn) {
      (*ppEntries)->m_pAddress = &material->texture;
      (*ppEntries)->m_pValue = material->texture;
      (*ppEntries)++;

      if (material->texture) {
        if (material->texture == CVehicleModelInfo::ms_pLightsTexture) {
          material->texture = CVehicleModelInfo::ms_pLightsOnTexture;
        } else {
          RwTexture *pTex = TextureMgr::FindOnTextureInDict(
              material, material->texture->dict);
          if (pTex) {
            material->texture = pTex;
          } else {
            LOG_VERBOSE("Expected an 'on' texture for {} but none found",
                        material->texture->name);
          }
        }
      }
      material->surfaceProps.ambient = gLightSurfProps.ambient;
    } else {
      pColor->red = matCol.off.r;
      pColor->green = matCol.off.g;
      pColor->blue = matCol.off.b;
      material->surfaceProps.ambient = gLightSurfPropsOff.ambient;
    }
  } else {
    CRGBA col = {255, 255, 255, 255};
    if (Carcols::GetColor(pCurVeh, material, col) &&
        CanStoreRestoreEntries(ppEntries, 1)) {
      RwRGBA *pColor = RpMaterialGetColor(material);
      (*ppEntries)->m_pAddress = pColor;
      (*ppEntries)->m_pValue = *reinterpret_cast<void **>(pColor);
      (*ppEntries)++;

      pColor->red = col.r;
      pColor->green = col.g;
      pColor->blue = col.b;
    }
  }

  return material;
}

bool ModelInfoMgr::IsMaterialAvailable(CVehicle *pVeh, eMaterialType type) {
  if (type >= 0 && type < eMaterialType::TotalMaterial) {
    auto &data = m_VehData.Get(pVeh);
    return data.m_MatAvail[type];
  }
  return false;
}
