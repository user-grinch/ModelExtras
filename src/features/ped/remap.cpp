#include "remap.h"
#include "pch.h"
#include <CCutsceneMgr.h>
#include <CKeyGen.h>
#include <CModelInfo.h>
#include <CPedModelInfo.h>
#include <CStreaming.h>
#include <CTxdStore.h>
#include <Renderware.h>
#include <rw/rpworld.h>
#include <rw/rwcore.h>

void PedRemap::CustomAssignRemapTxd(const char *txdName, uint16_t txdId) {
  if (txdName) {
    size_t len = strlen(txdName);

    if (len > 1) {
      if (isdigit(txdName[len - 1])) {
        if (strncmp(txdName, "peds", 3) == 0) {
          int arrayIndex = txdName[len - 1] - '0' - 1;
          if (arrayIndex < 4) {
            pedstxdIndexArray[arrayIndex] = txdId;
            CTxdStore::AddRef(pedstxdIndexArray[arrayIndex]);
            anyAdditionalPedsTxd = true;
            LOG_VERBOSE("Found additional peds {}.txd", arrayIndex + 1);
          } else {
            gLogger->error("peds*.txd limit is only up to 'peds5.txd'");
          }
        }
      }
    }
  }
  plugin::CallDynGlobal<const char *, uint16_t>(ORIGINAL_AssignRemapTxd,
                                                txdName, txdId);
}

void PedRemap::LoadAdditionalTxds() {
  txdsNotLoadedYet = false;
  bool anyRequest = false;

  if (anyAdditionalPedsTxd) {
    for (int i = 0; i < 4; ++i) {
      if (pedstxdIndexArray[i]) {
        CStreaming::RequestTxdModel(
            pedstxdIndexArray[i],
            (eStreamingFlags::GAME_REQUIRED | eStreamingFlags::KEEP_IN_MEMORY));
        // CStreaming::RequestTxdModel(pedstxdIndexArray[i], 8);
        LOG_VERBOSE("Loading additional txd id {}", (int)pedstxdIndexArray[i]);
      }
    }

    CStreaming::LoadAllRequestedModels(false);
    anyRequest = false;

    for (int i = 0; i < 4; ++i) {
      if (pedstxdIndexArray[i]) {
        pedstxdArray[i] = ((RwTexDictionary * (__cdecl *)(int))0x408340)(
            pedstxdIndexArray[i]); // size_t __cdecl getTexDictionary(int
                                   // txdIndex)
      }
    }
  }

  if (anyRequest) {
    CStreaming::LoadAllRequestedModels(false);
  }
}

RwTexture *__cdecl PedRemap::Custom_RwTexDictionaryFindNamedTexture(
    RwTexDictionary *dict, const char *name) {
  RwTexture *texture;

  texture = plugin::CallAndReturnDynGlobal<RwTexture *, RwTexDictionary *,
                                           const char *>(
      ORIGINAL_RwTexDictionaryFindNamedTexture, dict, name);
  // texture = RwTexDictionaryFindNamedTexture(dict, name);
  if (texture)
    return texture;

  if (anyAdditionalPedsTxd) {
    for (int i = 0; i < 4; ++i) {
      if (pedstxdArray[i]) {
        texture = RwTexDictionaryFindNamedTexture(pedstxdArray[i], name);
        if (texture)
          return texture;
      }
    }
  }
  // if (useLog) lg << name << " texture not found \n";
  if (txdsNotLoadedYet && anyAdditionalPedsTxd)
    LoadAdditionalTxds(); // looks not really safe, but tested a lot, if some
                          // problem by using more than 1 peds*.txd, do
                          // something here
  return nullptr;
}

const int totalOfDontRepeatIt = 30;
DontRepeatIt *dontRepeatIt[totalOfDontRepeatIt];
int curDontRepeatItIndex;

void PedRemap::Initialize() {
  srand(time(NULL));

  static std::list<std::pair<unsigned int *, unsigned int>> resetEntries;

  for (int i = 0; i < totalOfDontRepeatIt; ++i) {
    dontRepeatIt[i] = new DontRepeatIt;
  }
  curDontRepeatItIndex = 0;

  Events::initRwEvent += [] {
    memset(pedstxdArray, 0, sizeof(pedstxdArray));
    memset(pedstxdIndexArray, 0, sizeof(pedstxdIndexArray));

    ORIGINAL_AssignRemapTxd =
        injector::ReadMemory<uintptr_t>(0x5B62C2 + 1, true);
    ORIGINAL_AssignRemapTxd += (GetGlobalAddress(0x5B62C2) + 5);

    patch::RedirectCall(0x5B62C2, CustomAssignRemapTxd, true);

    ORIGINAL_RwTexDictionaryFindNamedTexture =
        injector::ReadMemory<uintptr_t>(0x4C7533 + 1, true);
    ORIGINAL_RwTexDictionaryFindNamedTexture +=
        (GetGlobalAddress(0x4C7533) + 5);

    patch::RedirectCall(0x4C7533, Custom_RwTexDictionaryFindNamedTexture, true);
    patch::RedirectCall(0x731733, Custom_RwTexDictionaryFindNamedTexture,
                        true); // for map etc
  };

  Events::initGameEvent += [] {
    if (!alreadyLoaded) {
      alreadyLoaded = true;
      if (txdsNotLoadedYet)
        LoadAdditionalTxds();
    }
  };

  Events::processScriptsEvent += [] {
    if (CCutsceneMgr::ms_running)
      cutsceneRunLastTime = CTimer::m_snTimeInMilliseconds;
  };

  Events::pedSetModelEvent += [](CPed *ped, int model) {
    if ((CTimer::m_snTimeInMilliseconds - cutsceneRunLastTime) > 3000)
      PedRemap::FindRemaps(ped);
  };

  Events::pedRenderEvent.before += [](CPed *ped) {
    if (ped->m_pRwClump && ped->m_pRwClump->object.type == rpCLUMP) {
      PedExtended &info = extData.Get(ped);

      if (info.curRemapNum[0] >= 0) {
        RpClumpForAllAtomics(
            ped->m_pRwClump,
            [](RpAtomic *atomic, void *data) {
              PedExtended *info = reinterpret_cast<PedExtended *>(data);
              if (atomic->geometry) {
                RpGeometryForAllMaterials(
                    atomic->geometry,
                    [](RpMaterial *material, void *data) {
                      PedExtended *info = reinterpret_cast<PedExtended *>(data);

                      resetEntries.push_back(std::make_pair(
                          reinterpret_cast<unsigned int *>(&material->texture),
                          *reinterpret_cast<unsigned int *>(
                              &material->texture)));

                      for (int i = 0; i < TEXTURE_LIMIT; ++i) {
                        if (info->curRemapNum[i] >= 0 &&
                            (int)info->originalRemap[i] > 0) {
                          if (info->originalRemap[i] == material->texture) {
                            if (!info->remaps[i].empty()) {
                              std::list<RwTexture *>::iterator it =
                                  info->remaps[i].begin();
                              advance(it, info->curRemapNum[i]);
                              material->texture = *it;
                            }
                          }
                        } else
                          break;
                      }
                      return material;
                    },
                    info);
              }
              return atomic;
            },
            &info);
      }
    }
  };

  Events::pedRenderEvent.after += [](CPed *ped) {
    for (auto &p : resetEntries)
      *p.first = p.second;
    resetEntries.clear();
  };
}

int GetIndexFromTexture(PedExtended *info, std::string name,
                        RwTexDictionary *pedTxdDic) {
  int i;
  for (i = 0; i < TEXTURE_LIMIT; ++i) {
    if ((int)info->originalRemap[i] > 0) {
      std::string nameStr = info->originalRemap[i]->name;
      if (nameStr.compare(name) == 0)
        return i;
    } else {
      info->originalRemap[i] =
          RwTexDictionaryFindNamedTexture(pedTxdDic, &name[0]);
      // lg << "added texture name to list: " << name << endl;
      return i;
    }
  }
  return -1;
}

void StoreSimpleRandom(PedExtended &info, int i) {
  info.curRemapNum[i] = RandomNumberInRange(-1, info.TotalRemapNum[i] - 1);
  dontRepeatIt[curDontRepeatItIndex]->lastNum[i] = info.curRemapNum[i];
  // lg << info.curRemapNum[i] << " get in simple " << endl;
}

void PedRemap::FindRemaps(CPed *ped) {
  PedExtended &info = extData.Get(ped);
  info = PedExtended(ped);

  CBaseModelInfo *pedModelInfo =
      (CBaseModelInfo *)CModelInfo::GetModelInfo(ped->m_nModelIndex);
  if (pedModelInfo) {
    TxdDef pedTxd =
        CTxdStore::ms_pTxdPool->m_pObjects[pedModelInfo->m_nTxdIndex];

    RwTexDictionary *pedTxdDic = pedTxd.m_pRwDictionary;
    if (pedTxdDic) {
      RwLinkList *objectList = &pedTxdDic->texturesInDict;
      if (!rwLinkListEmpty(objectList)) {
        RwTexture *texture;
        RwLLLink *current = rwLinkListGetFirstLLLink(objectList);
        RwLLLink *end = rwLinkListGetTerminator(objectList);

        std::string originalRemapName;

        current = rwLinkListGetFirstLLLink(objectList);
        while (current != end) {
          texture = rwLLLinkGetData(current, RwTexture, lInDictionary);

          std::size_t found;
          std::string name = texture->name;
          found = name.find("_remap");
          if (found != std::string::npos) {
            int index =
                GetIndexFromTexture(&info, name.substr(0, found), pedTxdDic);
            if (index != -1) // hit max
            {
              info.remaps[index].push_back(texture);
              info.TotalRemapNum[index]++;
            } else {
              gLogger->warn("Failed to add remap for texture {} due to {} "
                            "textures limit.",
                            texture->name, TEXTURE_LIMIT);
            }
          }

          current = rwLLLinkGetNext(current);
        }
        if (info.TotalRemapNum[0] > 0) {
          int lastNum = -1;

          for (int arrayIn = 0; arrayIn < totalOfDontRepeatIt; arrayIn++) {
            if (dontRepeatIt[arrayIn]->modelId == ped->m_nModelIndex) {
              for (int i = 0; i < TEXTURE_LIMIT; ++i) {
                if (info.TotalRemapNum[i] > 1) {
                  lastNum = dontRepeatIt[arrayIn]->lastNum[i];
                  do {
                    info.curRemapNum[i] = RandomNumberInRange(
                        -1, info.TotalRemapNum[i] - 1); //-1 previously
                  } while (info.curRemapNum[i] == lastNum);
                  dontRepeatIt[arrayIn]->lastNum[i] = info.curRemapNum[i];
                } else {
                  StoreSimpleRandom(info, i);
                }
              }
              return;
            }
          }
          dontRepeatIt[curDontRepeatItIndex]->modelId = ped->m_nModelIndex;
          curDontRepeatItIndex++;
          if (curDontRepeatItIndex >= totalOfDontRepeatIt)
            curDontRepeatItIndex = 0;
          for (int i = 0; i < TEXTURE_LIMIT; ++i) {
            if (info.TotalRemapNum[i] > 0) {
              StoreSimpleRandom(info, i);
            }
          }
          return;
        }
      }
    }
  }
}

extern "C" {
int ME_GetPedRemap(CPed *ped, int index) {
  PedExtended &info = PedRemap::extData.Get(ped);
  return info.curRemapNum[index];
}

void ME_SetPedRemap(CPed *ped, int index, int num) {
  PedExtended &info = PedRemap::extData.Get(ped);
  info.curRemapNum[index] = num;
  LOG_VERBOSE("MEAPI: {} called with index: {}, num: {}", __func__, index, num);
}

void ME_SetAllPedRemaps(CPed *ped, int num) {
  for (int i = 0; i < TEXTURE_LIMIT; ++i) {
    PedExtended &info = PedRemap::extData.Get(ped);
    info.curRemapNum[i] = num;
  }
  LOG_VERBOSE("MEAPI: {} called with num: {}", __func__, num);
}
}