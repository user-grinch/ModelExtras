#include "pch.h"
#include "remap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include "utils/modelinfomgr.h"
#include <rw/rwcore.h>
#include <rw/rpworld.h>
#include <algorithm>

void Remap::ReloadConfig()
{
    CBaseFeature::ReloadConfig();
    m_bEnabled = m_bActive;
}

void Remap::Init()
{
    ReloadConfig();
}

void Remap::EnsureRemapsLoaded(int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= 20000) return;
    auto itModel = xRemaps.find(modelIndex);
    if (itModel != xRemaps.end() && itModel->second.bRemapsLoaded) return;

    CBaseModelInfo *pModelInfo = CModelInfo::GetModelInfo(modelIndex);
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

    RemapData &data = xRemaps[modelIndex];
    data.bRemapsLoaded = true;

    // Collect all textures in the TXD by lowercase name
    std::unordered_map<std::string, RwTexture*> allTextures;
    RwTexDictionaryForAllTextures(pDict, [](RwTexture *pTex, void *pData)
    {
        auto *pMap = reinterpret_cast<std::unordered_map<std::string, RwTexture*>*>(pData);
        if (pTex && pTex->name[0])
        {
            std::string name = pTex->name;
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
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

void Remap::LoadRemaps(CVehicle* vehicle)
{
    if (!vehicle) return;
    EnsureRemapsLoaded(vehicle->m_nModelIndex);
}

bool Remap::HasRemaps(int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= 20000) return false;
    EnsureRemapsLoaded(modelIndex);
    auto it = xRemaps.find(modelIndex);
    return it != xRemaps.end() && !it->second.pTextures.empty();
}

int Remap::GetRemapCount(int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= 20000) return 0;
    EnsureRemapsLoaded(modelIndex);
    auto it = xRemaps.find(modelIndex);
    if (it == xRemaps.end() || it->second.pTextures.empty()) return 0;
    return static_cast<int>(it->second.pTextures.begin()->second.size());
}

int Remap::GetRemapIndex(CVehicle *pVeh)
{
    if (!pVeh) return -1;
    if (HasRemaps(pVeh->m_nModelIndex))
    {
        int count = GetRemapCount(pVeh->m_nModelIndex);
        if (count <= 0) return -1;
        RemapVehData &vehData = m_VehData.Get(pVeh);
        if (vehData.randomId == -1)
        {
            vehData.randomId = RandomNumberInRange(0, count - 1);
        }
        return ((vehData.randomId % count) + count) % count;
    }
    return pVeh->GetRemapIndex();
}

void Remap::SetRemapIndex(CVehicle *pVeh, int remapIndex)
{
    if (!pVeh) return;
    if (HasRemaps(pVeh->m_nModelIndex))
    {
        RemapVehData &vehData = m_VehData.Get(pVeh);
        vehData.randomId = remapIndex;
    }
    pVeh->SetRemap(remapIndex);
}

void Remap::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat)
{
    if (!m_bEnabled || !pVeh || !pMat || !pMat->texture || !pMat->texture->name)
    {
        return;
    }

    int model = pVeh->m_nModelIndex;
    EnsureRemapsLoaded(model);

    auto itModel = xRemaps.find(model);
    if (itModel == xRemaps.end() || itModel->second.pTextures.empty())
    {
        return;
    }

    RemapData &data = itModel->second;

    char lowerBuf[32];
    size_t len = 0;
    for (; len < sizeof(lowerBuf) - 1 && pMat->texture->name[len]; ++len) {
        lowerBuf[len] = static_cast<char>(std::tolower(static_cast<unsigned char>(pMat->texture->name[len])));
    }
    lowerBuf[len] = '\0';

    auto it = data.pTextures.find(lowerBuf);
    if (it == data.pTextures.end() || it->second.empty())
    {
        return;
    }

    int sz = static_cast<int>(it->second.size());
    if (sz <= 1)
    {
        return;
    }

    RemapVehData &vehData = m_VehData.Get(pVeh);
    if (vehData.randomId == -1)
    {
        vehData.randomId = RandomNumberInRange(0, sz - 1);
    }

    int chosen = ((vehData.randomId % sz) + sz) % sz;
    if (pMat->texture != it->second[chosen])
    {
        ModelInfoMgr::RegisterRestore(&pMat->texture, pMat->texture);
        pMat->texture = it->second[chosen];
    }
}