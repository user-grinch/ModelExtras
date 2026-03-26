#include "pch.h"
#include "remap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include "utils/meevents.h"
#include "texmgr.h"
#include <rw/rwcore.h>
#include <rw/rpworld.h>

void Remap::LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type)
{
    if (pModelInfo)
    {
        CTxdStore::PushCurrentTxd();
        CTxdStore::SetCurrentTxd(pModelInfo->m_nTxdIndex);
        static RwTexDictionary *pDict;
        pDict = RwTexDictionaryGetCurrent();
        RwTexDictionaryForAllTextures(pDict, [](RwTexture *pTex, void *pData)
        { 
            int model = *(int*)pData;
            std::string name = pTex->name;
            if (name.starts_with("#") || name.starts_with("remap")) {
                return pTex;
            }
            std::size_t remapPos = name.find("_remap");
            std::size_t bloodPos = name.find("_bld");
            std::string orgName = name.substr(0, std::min(std::min(remapPos, bloodPos), name.size()));
            bool isRemapTex = remapPos != std::string::npos;
            bool isBloodTex = bloodPos != std::string::npos;

            if (isRemapTex || isBloodTex) {
                RemapData &data = xRemaps[model];
                if (data.pTextures.empty()) {
                    std::string bloodName = orgName + "_bld";
                    RwTexture *pOrgTex = TextureMgr::FindInDict(orgName, pDict);
                    RwTexture *pBloodTex = TextureMgr::FindInDict(bloodName, pDict);
                    data.pTextures[orgName].push_back({pOrgTex, pBloodTex});
                }
            }
            
            if (isRemapTex && !isBloodTex)
            {
                RemapData &data = xRemaps[model];
                std::string bloodName = name + "_bld";
                RwTexture *pBloodTex = TextureMgr::FindInDict(bloodName, pDict);
                data.pTextures[orgName].push_back({pTex, pBloodTex});
            }
			return pTex; 
        }, &model);
        CTxdStore::PopCurrentTxd();
    }
}

std::vector<std::pair<unsigned int *, unsigned int>> pOriginalTextures;
std::map<void *, int> pRandom;
std::map<void *, bool> pBloodState;

void Remap::Initialize()
{
    Events::vehicleRenderEvent.before += [](CVehicle *ptr)
    {
        BeforeRender(reinterpret_cast<void *>(ptr), eModelEntityType::Vehicle);
    };

    // Events::pedRenderEvent.before += [](CPed *pPed)
    // {
    //     BeforeRender(reinterpret_cast<void *>(pPed), eModelEntityType::Ped);
    // };

    // These two events need to be hooked later
    Events::initGameEvent += []()
    {
        static bool init = false;
        if (init)
        {
            return;
        }
        MEEvents::weaponRenderEvent.before += [](CPed *pPed)
        {
            CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nSelectedWepSlot];
            if (pWeapon && pWeapon->m_eWeaponType != eWeaponType::WEAPONTYPE_UNARMED)
            {
                BeforeRender(reinterpret_cast<void *>(pWeapon), eModelEntityType::Weapon);
            }
        };

        MEEvents::weaponRenderEvent.after += [](CPed *pPed)
        {
            CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nSelectedWepSlot];
            if (pWeapon && pWeapon->m_eWeaponType != eWeaponType::WEAPONTYPE_UNARMED)
            {
                AfterRender(reinterpret_cast<void *>(pWeapon), eModelEntityType::Weapon);
            }
        };
        init = true;
    };

    Events::vehicleRenderEvent.after += [](CVehicle *ptr)
    {
        AfterRender(reinterpret_cast<void *>(ptr), eModelEntityType::Vehicle);
    };

    // Events::pedRenderEvent.after += [](CPed *pPed)
    // {
    //     AfterRender(reinterpret_cast<void *>(pPed), eModelEntityType::Ped);
    // };

    MEEvents::weaponRemoveEvent.before += [](CPed *pPed, int model)
    {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nSelectedWepSlot];
        if (pWeapon)
        {
            if (pRandom.contains(pWeapon))
            {
                pRandom.erase(pRandom.find(pWeapon));
            }

            if (pBloodState.contains(pWeapon))
            {
                pBloodState.erase(pBloodState.find(pWeapon));
            }
        }
    };

    MEEvents::weaponInitEvent.before += [](CWeapon *pWeapon, int type, int ammo, CPed *owner)
    {
        if (pWeapon)
        {
            if (pRandom.contains(pWeapon))
            {
                pRandom.erase(pRandom.find(pWeapon));
            }

            if (pBloodState.contains(pWeapon))
            {
                pBloodState.erase(pBloodState.find(pWeapon));
            }
        }
    };
}

void Remap::AfterRender(void *, eModelEntityType)
{
    for (auto &pEnt : pOriginalTextures)
    {
        *pEnt.first = pEnt.second;
    }
    pOriginalTextures.clear();
}

bool Remap::GetKilledState(CWeapon *pWeapon)
{
    if (pWeapon == nullptr)
    {
        return false;
    }

    if (pBloodState[pWeapon])
    {
        return true;
    }

    static CPed *lastKilled = nullptr;
    auto player = FindPlayerPed();
    if (player && player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType == pWeapon->m_eWeaponType)
    {
        CPed *pPed = static_cast<CPed *>(player->m_pDamageEntity);
        if (!pPed)
        {
            pPed = static_cast<CPed *>(player->m_pLastEntityDamage);
        }
        if (pPed && pPed->m_nType == ENTITY_TYPE_PED && !pPed->IsAlive() && pPed != lastKilled)
        {
            pBloodState[pWeapon] = true;
            lastKilled = pPed;
        }
    }
    return pBloodState[pWeapon];
}

void Remap::BeforeRender(void *ptr, eModelEntityType type)
{
    int model = Util::GetEntityModel(ptr, type);
    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(model);

    RemapData &data = xRemaps[model];
    if (!data.bRemapsLoaded)
    {
        LoadRemaps(pModelInfo, model, type);
        data.bRemapsLoaded = true;
    }

    if (data.pTextures.empty())
    {
        return;
    }

    if (type == eModelEntityType::Weapon)
    {
        data.bUseBlood = GetKilledState(static_cast<CWeapon *>(ptr));
    }

    data.pCurPtr = ptr;
    RpClumpForAllAtomics(pModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data)
                         {
        if (atomic->geometry) {
            RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *mat, void *data) {
                if (!mat->texture) {
                    return mat;
                }

                std::string name = mat->texture->name;
                RemapData *pData = reinterpret_cast<RemapData*>(data);

                if (pData->pTextures[name].empty()) {
                    return mat;
                }
                int sz = pData->pTextures[name].size();
                if (pRandom.find(pData->pCurPtr) == pRandom.end() || pRandom[pData->pCurPtr] >= sz) {
                    pRandom[pData->pCurPtr] = RandomNumberInRange(0, sz-1);
                }

                pOriginalTextures.push_back({reinterpret_cast<unsigned int*>(&mat->texture), *reinterpret_cast<unsigned int *>(&mat->texture)});

                if (pData->bUseBlood && pData->pTextures[name][pRandom[pData->pCurPtr]].pBlood) {
                    mat->texture = pData->pTextures[name][pRandom[pData->pCurPtr]].pBlood;
                } else {
                    mat->texture = pData->pTextures[name][pRandom[pData->pCurPtr]].pNormal;
                }
                mat->texture->refCount++;

                return mat;
            }, data);
        }
        return atomic; 
    }, &data);
}