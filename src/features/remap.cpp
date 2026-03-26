#include "pch.h"
#include "remap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include "utils/texmgr.h"
#include <rw/rwcore.h>
#include <rw/rpworld.h>

void Remap::LoadRemaps(CVehicle* vehicle)
{
    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
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
            if (remapPos == std::string::npos) {
                return pTex;
            }

            std::string orgName = name.substr(0, remapPos);
            RemapData &data = xRemaps[model];

            // Add original texture if it's the first remap texture found for it
            if (data.pTextures.find(orgName) == data.pTextures.end()) {
                RwTexture *pOrgTex = TextureMgr::FindInDict(orgName, pDict);
                if (pOrgTex) {
                    data.pTextures[orgName].push_back(pOrgTex);
                }
            }
            
            // Add the remap texture itself
            data.pTextures[orgName].push_back(pTex);

			return pTex; 
        }, &vehicle->m_nModelIndex);
        CTxdStore::PopCurrentTxd();
    }
}

std::vector<std::pair<unsigned int *, unsigned int>> pOriginalTextures;
std::map<void *, int> pRandom;

void Remap::Init()
{
    Events::vehicleRenderEvent.before += [](CVehicle *vehicle)
    {
        BeforeRender(vehicle);
    };

    Events::vehicleRenderEvent.after += [](CVehicle *vehicle)
    {
        AfterRender(vehicle);
    };
}

void Remap::AfterRender(CVehicle* vehicle)
{
    for (auto &pEnt : pOriginalTextures)
    {
        *pEnt.first = pEnt.second;
    }
    pOriginalTextures.clear();
}

void Remap::BeforeRender(CVehicle* vehicle)
{
    int model = vehicle->m_nModelIndex;
    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(model);

    RemapData &data = xRemaps[model];
    if (!data.bRemapsLoaded)
    {
        LoadRemaps(vehicle);
        data.bRemapsLoaded = true;
    }

    if (data.pTextures.empty())
    {
        return;
    }

    data.pCurPtr = vehicle;
    RpClumpForAllAtomics(pModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data)
    {
        if (atomic->geometry) {
            RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *mat, void *data) {
                if (!mat->texture) {
                    return mat;
                }

                std::string name = mat->texture->name;
                RemapData *pData = reinterpret_cast<RemapData*>(data);

                if (pData->pTextures.find(name) == pData->pTextures.end()) {
                    return mat;
                }
                
                int sz = pData->pTextures[name].size();
                if (pRandom.find(pData->pCurPtr) == pRandom.end() || pRandom[pData->pCurPtr] >= sz) {
                    pRandom[pData->pCurPtr] = RandomNumberInRange(0, sz-1);
                }

                pOriginalTextures.push_back({reinterpret_cast<unsigned int*>(&mat->texture), *reinterpret_cast<unsigned int *>(&mat->texture)});
                
                mat->texture = pData->pTextures[name][pRandom[pData->pCurPtr]];
                mat->texture->refCount++;

                return mat;
            }, data);
        }
        return atomic; 
    }, &data);
}