#include "pch.h"
#include "randomremap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include <CKeyGen.h>
#define MAX_REMAPS 32
#define NODE_ID "x_ranmap"

RandomRemapFeature RandomRemap;

void RandomRemapFeature::LoadRemaps(CBaseModelInfo *pModelInfo, int model, eNodeEntityType type)
{
	if (pModelInfo)
	{
		TxdDef pedTxd = CTxdStore::ms_pTxdPool->m_pObjects[pModelInfo->m_nTxdIndex];

		RwTexDictionary * pedTxdDic = pedTxd.m_pRwDictionary;
		if (pedTxdDic)
		{
			RwLinkList *objectList = &pedTxdDic->texturesInDict;
			if (!rwLinkListEmpty(objectList))
			{
				RwLLLink *current = rwLinkListGetFirstLLLink(objectList);
				RwLLLink *end = rwLinkListGetTerminator(objectList);

				current = rwLinkListGetFirstLLLink(objectList);
				while (current != end)
				{
					RwTexture *pTexture = rwLLLinkGetData(current, RwTexture, lInDictionary);

					std::string name = pTexture->name;
					std::size_t found = name.find("_remap");

					if (found != std::string::npos)
					{
                        std::string ogRemap = name.substr(0, found);
                        RemapData &data = xRemaps.Get(model);
                        if (data.m_pTextures[ogRemap].size() == 0) {
                            RwTexture *ogTexture = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(ogRemap.c_str()));
                            data.m_pTextures[ogRemap].push_back(ogTexture);
                        }

						data.m_pTextures[ogRemap].push_back(pTexture);
					}
					current = rwLLLinkGetNext(current);
				}
			}
		}
	}
}
  
static std::vector<std::pair<unsigned int *, unsigned int>> m_pOriginalTextures;
static std::map<void*, int> m_pRandom;

static CdeclEvent <AddressList<0x5E7859, H_CALL>, PRIORITY_BEFORE, ArgPickN<CPed*, 0>, void(CPed*)> weaponRenderEvent;
static ThiscallEvent <AddressList<0x43D821, H_CALL,
                                  0x43D939, H_CALL,
                                  0x45CC78, H_CALL,
                                  0x47D3AD, H_CALL,
                                  0x5E3B53, H_CALL,
                                  0x5E5F14, H_CALL,
                                  0x5E6150, H_CALL,
                                  0x5E6223, H_CALL,
                                  0x5E6327, H_CALL,
                                  0x63072E, H_CALL,
                                  0x5E6483, H_CALL,
                                  0x6348FC, H_CALL>, PRIORITY_BEFORE, ArgPick2N<void*, 0, int, 1>, void(void*, int)> weaponRemoveEvent;

void RandomRemapFeature::Initialize() {
    Events::vehicleRenderEvent.before += [this](CVehicle* ptr) {
        BeforeRender(reinterpret_cast<void*>(ptr), eNodeEntityType::Vehicle);
    };

    Events::pedRenderEvent.before += [this](CPed* pPed) {
        BeforeRender(reinterpret_cast<void*>(pPed), eNodeEntityType::Ped);
    };

    weaponRenderEvent.before += [this](CPed* pPed) {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            BeforeRender(reinterpret_cast<void*>(pWeapon), eNodeEntityType::Weapon);
        }
    };
    
    Events::vehicleRenderEvent.after += [this](CVehicle* ptr) {
        AfterRender(reinterpret_cast<void*>(ptr), eNodeEntityType::Vehicle);
    };

    Events::pedRenderEvent.after += [this](CPed* pPed) {
        AfterRender(reinterpret_cast<void*>(pPed), eNodeEntityType::Ped);
    };

    weaponRenderEvent.after += [this](CPed* pPed) {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            AfterRender(reinterpret_cast<void*>(pWeapon), eNodeEntityType::Weapon);
        }
    };

    weaponRemoveEvent.before += [this](void* ptr, int model) {
        if(m_pRandom.contains(ptr)) {
            m_pRandom.erase(m_pRandom.find(ptr));
        }
    };
}

void RandomRemapFeature::AfterRender(void* ptr, eNodeEntityType type) {
    for (auto &e : m_pOriginalTextures) {
		*e.first = e.second;
    }
	m_pOriginalTextures.clear();
}

void RandomRemapFeature::BeforeRender(void* ptr, eNodeEntityType type) {
    int model = Util::GetEntityModel(ptr, type);
    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(model);

    RemapData &data = xRemaps.Get(model);
    if (!data.m_bRemapsLoaded) {
        LoadRemaps(pModelInfo, model, type);
        data.m_bRemapsLoaded = true;
    }

    if (data.m_pTextures.empty()) {
        return;
    }

    data.curPtr = ptr;
    RpClumpForAllAtomics(pModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data) {
        if (atomic->geometry) {
            RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *mat, void *data) {
                std::string name = mat->texture->name;
                RemapData *pData = reinterpret_cast<RemapData*>(data);

                if (pData->m_pTextures[name].empty()) {
                    return mat;
                }
                if (m_pRandom.find(pData->curPtr) == m_pRandom.end()) {
                    m_pRandom[pData->curPtr] = Random(0u, pData->m_pTextures[name].size()-1);
                }
                m_pOriginalTextures.push_back({reinterpret_cast<unsigned int *>(&mat->texture), *reinterpret_cast<unsigned int *>(&mat->texture)});
                mat->texture = pData->m_pTextures[name][m_pRandom[pData->curPtr]];
                return mat;
            }, data);
        }
        return atomic;
    }, &data);
}