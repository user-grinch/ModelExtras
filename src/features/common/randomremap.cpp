#include "pch.h"
#include "randomremap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include <CKeyGen.h>
#define MAX_REMAPS 32
#define NODE_ID "x_ranmap"

RandomRemapFeature RandomRemap;

struct RemapData {
    bool m_bInit;
    unsigned int rand;
    std::vector<RwTexture*> m_pTextures;
};

void RandomRemapFeature::Process(RwFrame* frame, void* ptr, eNodeEntityType type) {
    FrameData &data = xFrame.Get(frame);

    if (data.m_bInit) {
        return;
    }

    std::string name = GetFrameNodeName(frame);
    if (name.find(NODE_ID) != std::string::npos) {
        std::string str = Util::GetRegexVal(name, NODE_ID"_(.*)", "");

        RpClump *pClump;
        RemapData remapData;

        CBaseModelInfo *pModelInfo = nullptr;
        if (type == eNodeEntityType::Weapon) {
            CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(reinterpret_cast<CWeapon*>(ptr)->m_eWeaponType, 
                                                                    FindPlayerPed()->GetWeaponSkill(reinterpret_cast<CWeapon*>(ptr)->m_eWeaponType));
            if (pWeaponInfo){
                pModelInfo = CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1);
            }
        } else {
            pModelInfo = CModelInfo::GetModelInfo(reinterpret_cast<CEntity*>(ptr)->m_nModelIndex);
        }


        if (pModelInfo) {
            pClump = pModelInfo->m_pRwClump;

            TxdDef pedTxd = CTxdStore::ms_pTxdPool->m_pObjects[pModelInfo->m_nTxdIndex];
            RwTexDictionary *pedTxdDic = pedTxd.m_pRwDictionary;
            if (pedTxdDic) {
                RwTexture *ptr = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(str.c_str()));
                remapData.m_pTextures.push_back(ptr);
                for (int i = 1; i != MAX_REMAPS; i++) {
                    RwTexture *ptr = RwTexDictionaryFindHashNamedTexture(pedTxdDic, 
                                        CKeyGen::GetUppercaseKey((str + "_" + std::to_string(i)).c_str()));

                    if (ptr) {
                        remapData.m_pTextures.push_back(ptr);
                    }
                }
            }
        }

        remapData.rand = Random(0u, remapData.m_pTextures.size()-1);
        RpClumpForAllAtomics(pClump, [](RpAtomic *atomic, void *data) {
            if (atomic->geometry) {
                RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *material, void *data) {
                    RemapData *pData = reinterpret_cast<RemapData*>(data);
                    if (material->texture == pData->m_pTextures[0]) {
                        material->texture = pData->m_pTextures[pData->rand];
                    }
                    return material;
                }, data);
            }
            return atomic;
        }, &remapData);

        data.m_bInit = true;
    }
}