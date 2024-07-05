#include "pch.h"
#include "remap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include <CKeyGen.h>
#define MAX_REMAPS 32
#define NODE_ID "x_ranmap"

void Remap::LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type)
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
                    std::size_t bloodFound = name.find("_b");

					if (found != std::string::npos && bloodFound == std::string::npos)
					{
                        std::string ogRemap = name.substr(0, found);
                        RemapData &data = xRemaps.Get(model);

                        if (data.m_pTextures[ogRemap].size() == 0) {
                            std::string blood = ogRemap + "_b";
                            RwTexture *tex = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(ogRemap.c_str()));
                            RwTexture *texblood = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(blood.c_str()));
                            data.m_pTextures[ogRemap].push_back({tex, texblood});
                        }

                        std::string blood = name + "_b";
                        RwTexture *texblood = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(blood.c_str()));
						data.m_pTextures[ogRemap].push_back({pTexture, texblood});
					}
					current = rwLLLinkGetNext(current);
				}
			}
		}
	}
}
  
static std::vector<std::pair<unsigned int *, unsigned int>> m_pOriginalTextures;
static std::map<void*, int> m_pRandom;
static std::map<void*, bool> m_pBloodState;

void Remap::Initialize() {
    Events::vehicleRenderEvent.before += [](CVehicle* ptr) {
        BeforeRender(reinterpret_cast<void*>(ptr), eModelEntityType::Vehicle);
    };

    Events::pedRenderEvent.before += [](CPed* pPed) {
        BeforeRender(reinterpret_cast<void*>(pPed), eModelEntityType::Ped);
    };

    weaponRenderEvent.before += [](CPed* pPed) {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            BeforeRender(reinterpret_cast<void*>(pWeapon), eModelEntityType::Weapon);
        }
    };
    
    Events::vehicleRenderEvent.after += [](CVehicle* ptr) {
        AfterRender(reinterpret_cast<void*>(ptr), eModelEntityType::Vehicle);
    };

    Events::pedRenderEvent.after += [](CPed* pPed) {
        AfterRender(reinterpret_cast<void*>(pPed), eModelEntityType::Ped);
    };

    weaponRenderEvent.after += [](CPed* pPed) {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            AfterRender(reinterpret_cast<void*>(pWeapon), eModelEntityType::Weapon);
        }
    };

    weaponRemoveEvent.before += [](CPed* pPed, int model) {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nActiveWeaponSlot];
        if (pWeapon) {
            if(m_pRandom.contains(pWeapon)) {
                m_pRandom.erase(m_pRandom.find(pWeapon));
            }

            if(m_pBloodState.contains(pWeapon)) {
                m_pBloodState.erase(m_pBloodState.find(pWeapon));
            }
        }
    };
}

void Remap::AfterRender(void* ptr, eModelEntityType type) {
    for (auto &e : m_pOriginalTextures) {
		*e.first = e.second;
    }
	m_pOriginalTextures.clear();
}

bool Remap::GetKilledState(CWeapon *pWeapon) {
    if (m_pBloodState[pWeapon]) {
        return true;
    }

    bool state = false;
    static CPed *lastKilled = nullptr;

    auto player = FindPlayerPed();
    if (player && player->m_aWeapons[player->m_nActiveWeaponSlot].m_eWeaponType == pWeapon->m_eWeaponType) {
        CPed *pPed = static_cast<CPed*>(player->m_pDamageEntity);
        if (!pPed) {
            pPed = static_cast<CPed*>(player->m_pLastEntityDamage);
        }
        if (pPed && pPed->m_nType == ENTITY_TYPE_PED && !pPed->IsAlive() && pPed != lastKilled) {
            state = true;
            m_pBloodState[pWeapon] = true;
            lastKilled = pPed;
        }
    }
    return state;
}

void Remap::BeforeRender(void* ptr, eModelEntityType type) {
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
    
    if (type == eModelEntityType::Weapon) {
        data.useBlood = GetKilledState(static_cast<CWeapon*>(ptr));
    }

    data.curPtr = ptr;
    RpClumpForAllAtomics(pModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data) {
        if (atomic->geometry) {
            RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *mat, void *data) {
                if (!mat->texture) {
                    return mat;
                }

                std::string name = mat->texture->name;
                RemapData *pData = reinterpret_cast<RemapData*>(data);

                if (pData->m_pTextures[name].empty()) {
                    return mat;
                }
                int sz = pData->m_pTextures[name].size();
                if (m_pRandom.find(pData->curPtr) == m_pRandom.end() || m_pRandom[pData->curPtr] >= sz) {
                    m_pRandom[pData->curPtr] = RandomNumberInRange(0, sz-1);
                }

                m_pOriginalTextures.push_back({reinterpret_cast<unsigned int *>(&mat->texture), *reinterpret_cast<unsigned int *>(&mat->texture)});

                if (pData->useBlood && pData->m_pTextures[name][m_pRandom[pData->curPtr]].m_pBlood) {
                    mat->texture = pData->m_pTextures[name][m_pRandom[pData->curPtr]].m_pBlood;
                } else {
                    mat->texture = pData->m_pTextures[name][m_pRandom[pData->curPtr]].m_pNormal;
                }

                return mat;
            }, data);
        }
        return atomic;
    }, &data);
}