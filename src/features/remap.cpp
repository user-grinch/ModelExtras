#include "pch.h"
#include "remap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include "utils/modelinfomgr.h"
#include <rw/rwcore.h>
#include <rw/rpworld.h>
#include <algorithm>

void Remap::Init()
{
    m_bEnabled = true;
    Events::vehicleDtorEvent += [](CVehicle *vehicle)
    {
        pRandom.erase(vehicle);
    };
}

void Remap::LoadRemaps(CVehicle* vehicle)
{
    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
    if (!pModelInfo) return;

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(pModelInfo->m_nTxdIndex);

    RwTexDictionary *pDict = nullptr;
    if (CTxdStore::ms_pTxdPool && pModelInfo->m_nTxdIndex >= 0)
    {
        auto *pDef = CTxdStore::ms_pTxdPool->GetAt(pModelInfo->m_nTxdIndex);
        if (pDef)
        {
            pDict = pDef->m_pRwDictionary;
        }
    }
    if (!pDict)
    {
        pDict = RwTexDictionaryGetCurrent();
    }
    if (!pDict)
    {
        CTxdStore::PopCurrentTxd();
        return;
    }

    int model = vehicle->m_nModelIndex;
    RemapData &data = xRemaps[model];

    // Collect all textures in the TXD by lowercase name
    std::map<std::string, RwTexture*> allTextures;
    RwTexDictionaryForAllTextures(pDict, [](RwTexture *pTex, void *pData)
    {
        auto *pMap = reinterpret_cast<std::map<std::string, RwTexture*>*>(pData);
        if (pTex && pTex->name[0])
        {
            std::string name = pTex->name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            (*pMap)[name] = pTex;
        }
        return pTex;
    }, &allTextures);

    // Group textures into base -> [base, remap1, remap2, ...]
    for (const auto &[name, pTex] : allTextures)
    {
        if (name.starts_with("#") || name.starts_with("remap"))
            continue;

        std::size_t remapPos = name.find("_remap");
        if (remapPos == std::string::npos)
            continue;

        std::string orgName = name.substr(0, remapPos);

        // Add original texture first if present and not already added
        if (data.pTextures.find(orgName) == data.pTextures.end())
        {
            auto itOrg = allTextures.find(orgName);
            if (itOrg != allTextures.end())
            {
                data.pTextures[orgName].push_back(itOrg->second);
            }
        }

        data.pTextures[orgName].push_back(pTex);
    }
    CTxdStore::PopCurrentTxd();
}

void Remap::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat)
{
    if (!m_bEnabled || !pVeh || !pMat || !pMat->texture)
    {
        return;
    }

    int model = pVeh->m_nModelIndex;
    RemapData &data = xRemaps[model];
    if (!data.bRemapsLoaded)
    {
        LoadRemaps(pVeh);
        data.bRemapsLoaded = true;
    }

    if (data.pTextures.empty())
    {
        return;
    }

    std::string name = pMat->texture->name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    auto it = data.pTextures.find(name);
    if (it == data.pTextures.end() || it->second.empty())
    {
        return;
    }

    int sz = static_cast<int>(it->second.size());
    if (sz <= 1)
    {
        return;
    }

    if (pRandom.find(pVeh) == pRandom.end())
    {
        pRandom[pVeh] = RandomNumberInRange(0, sz - 1);
    }

    int chosen = pRandom[pVeh] % sz;
    if (pMat->texture != it->second[chosen])
    {
        ModelInfoMgr::RegisterRestore(&pMat->texture, pMat->texture);
        pMat->texture = it->second[chosen];
    }
}