#include "pch.h"
#include "remap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include "meevents.h"

void Remap::LoadRemaps(CBaseModelInfo *pModelInfo, int model, eModelEntityType type)
{
    if (pModelInfo)
    {
        CTxdStore::PushCurrentTxd();
        CTxdStore::SetCurrentTxd(pModelInfo->m_nTxdIndex);
        RwTexDictionaryForAllTextures(RwTexDictionaryGetCurrent(), [](RwTexture *pTex, void *pData)
                                      { 
            int model = *(int*)pData;
            std::string name = pTex->name;
            std::size_t remapPos = name.find("_remap");
            std::size_t bloodPos = name.find("_bld");
            std::string orgName = name.substr(0, std::min(std::min(remapPos, bloodPos), name.size()));
            
            // remap but not blood one
            if (remapPos != std::string::npos && bloodPos == std::string::npos)
            {
                RemapData &data = xRemaps.Get(model);
                std::string bloodName = name + "_bld";
                RwTexture *pBloodTex = RwTexDictionaryFindNamedTexture(RwTexDictionaryGetCurrent(), bloodName.c_str());
                data.m_pTextures[orgName].push_back({pTex, pBloodTex});
            }

            // edge case: original blood one
            if (remapPos == std::string::npos && bloodPos != std::string::npos) {
                RemapData &data = xRemaps.Get(model);
                RwTexture *orgTex = RwTexDictionaryFindNamedTexture(RwTexDictionaryGetCurrent(), orgName.c_str());
                data.m_pTextures[orgName].push_back({orgTex, pTex});
            }
										return pTex; }, &model);
        CTxdStore::PopCurrentTxd();
    }
}

static std::vector<std::pair<unsigned int *, unsigned int>> m_pOriginalTextures;
static std::map<void *, int> m_pRandom;
static std::map<void *, bool> m_pBloodState;

void Remap::Initialize()
{
    Events::vehicleRenderEvent.before += [](CVehicle *ptr)
    {
        BeforeRender(reinterpret_cast<void *>(ptr), eModelEntityType::Vehicle);
    };

    Events::pedRenderEvent.before += [](CPed *pPed)
    {
        BeforeRender(reinterpret_cast<void *>(pPed), eModelEntityType::Ped);
    };

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

    Events::pedRenderEvent.after += [](CPed *pPed)
    {
        AfterRender(reinterpret_cast<void *>(pPed), eModelEntityType::Ped);
    };

    MEEvents::weaponRemoveEvent.before += [](CPed *pPed, int model)
    {
        CWeapon *pWeapon = &pPed->m_aWeapons[pPed->m_nSelectedWepSlot];
        if (pWeapon)
        {
            if (m_pRandom.contains(pWeapon))
            {
                m_pRandom.erase(m_pRandom.find(pWeapon));
            }

            if (m_pBloodState.contains(pWeapon))
            {
                m_pBloodState.erase(m_pBloodState.find(pWeapon));
            }
        }
    };

    MEEvents::weaponInitEvent.before += [](CWeapon *pWeapon, int type, int ammo, CPed *owner)
    {
        if (pWeapon)
        {
            if (m_pRandom.contains(pWeapon))
            {
                m_pRandom.erase(m_pRandom.find(pWeapon));
            }

            if (m_pBloodState.contains(pWeapon))
            {
                m_pBloodState.erase(m_pBloodState.find(pWeapon));
            }
        }
    };
}

void Remap::AfterRender(void *ptr, eModelEntityType type)
{
    for (auto &e : m_pOriginalTextures)
    {
        *e.first = e.second;
    }
    m_pOriginalTextures.clear();
}

bool Remap::GetKilledState(CWeapon *pWeapon)
{
    if (!pWeapon)
    {
        return false;
    }

    if (m_pBloodState[pWeapon])
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
            m_pBloodState[pWeapon] = true;
            lastKilled = pPed;
        }
    }
    return m_pBloodState[pWeapon];
}

void Remap::BeforeRender(void *ptr, eModelEntityType type)
{
    int model = Util::GetEntityModel(ptr, type);
    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(model);

    RemapData &data = xRemaps.Get(model);
    if (!data.m_bRemapsLoaded)
    {
        LoadRemaps(pModelInfo, model, type);
        data.m_bRemapsLoaded = true;
    }

    if (data.m_pTextures.empty())
    {
        return;
    }

    if (type == eModelEntityType::Weapon)
    {
        data.useBlood = GetKilledState(static_cast<CWeapon *>(ptr));
    }

    data.curPtr = ptr;
    RpClumpForAllAtomics(pModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data)
                         {
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

                m_pOriginalTextures.push_back({reinterpret_cast<unsigned int*>(&mat->texture), *reinterpret_cast<unsigned int *>(&mat->texture)});

                if (pData->useBlood && pData->m_pTextures[name][m_pRandom[pData->curPtr]].m_pBlood) {
                    mat->texture = pData->m_pTextures[name][m_pRandom[pData->curPtr]].m_pBlood;
                } else {
                    mat->texture = pData->m_pTextures[name][m_pRandom[pData->curPtr]].m_pNormal;
                }

                return mat;
            }, data);
        }
        return atomic; }, &data);
}