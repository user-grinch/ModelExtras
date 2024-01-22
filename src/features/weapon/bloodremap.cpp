#include "pch.h"
#include "bloodremap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include <CKeyGen.h>
#include <CStats.h>
#define MAX_REMAPS 32

BloodRemapFeature BloodRemap;

void BloodRemapFeature::Initialize(RwFrame* pFrame, CWeapon* pWeapon) {
    FrameData &data = xData.Get(pWeapon);
    std::string name = GetFrameNodeName(pFrame);
    std::string str = Util::GetRegexVal(name, "x_remap_(.*)", "");

    CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(pWeapon->m_eWeaponType, FindPlayerPed()->GetWeaponSkill(pWeapon->m_eWeaponType));
    if (pWeaponInfo){
        CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
        if (pWeaponModelInfo) {
            TxdDef pedTxd = CTxdStore::ms_pTxdPool->m_pObjects[pWeaponModelInfo->m_nTxdIndex];
            RwTexDictionary *pedTxdDic = pedTxd.m_pRwDictionary;
            if (pedTxdDic) {
                RwTexture *ptr = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(str.c_str()));
                data.m_Textures[name].m_pFrames.push_back(ptr);
                for (int i = 1; i != MAX_REMAPS; i++) {
                    RwTexture *ptr = RwTexDictionaryFindHashNamedTexture(pedTxdDic, 
                                        CKeyGen::GetUppercaseKey((str + "_" + std::to_string(i)).c_str()));

                    if (ptr) {
                        data.m_Textures[name].m_pFrames.push_back(ptr);
                    }
                    else {
                        data.m_Textures[name].m_nTotalRemaps = i-1;
                        break;
                    }
                }
            }
        }
    }
}

void BloodRemapFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
    FrameData &data = xData.Get(pWeapon);
    std::string name = GetFrameNodeName(frame);
    data.m_CurNode = name;
    if (name.find("x_remap") != std::string::npos) {

        if (!data.m_Textures[name].m_bInit) {
            Initialize(frame, pWeapon);
            data.m_Textures[name].m_bInit = true;
        }

        auto player = FindPlayerPed();
        if (player && player->m_aWeapons[player->m_nActiveWeaponSlot].m_eWeaponType == pWeapon->m_eWeaponType) {
            CPed *pPed = static_cast<CPed*>(player->m_pDamageEntity);
            if (!pPed) {
                pPed = static_cast<CPed*>(player->m_pLastEntityDamage);
            }
            if (pPed && pPed->m_nType == ENTITY_TYPE_PED && !pPed->IsAlive() && pPed != data.m_pLastKilledEntity) {
                for (auto &e: data.m_Textures) {
                    if (e.second.m_nCurRemap < e.second.m_nTotalRemaps) {
                        e.second.m_nCurRemap++;
                    }
                }
                data.m_pLastKilledEntity = pPed;
            }
        }

        CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(pWeapon->m_eWeaponType, FindPlayerPed()->GetWeaponSkill(pWeapon->m_eWeaponType));
        if (!pWeaponInfo) return;

        CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
        if (!pWeaponModelInfo) return;

        RpClumpForAllAtomics(pWeaponModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data) {
            if (atomic->geometry) {
                RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *material, void *data) {
                    FrameData *pData = reinterpret_cast<FrameData*>(data);
                    if (material->texture == pData->m_Textures[pData->m_CurNode].m_pFrames[0]) {
                        material->texture = pData->m_Textures[pData->m_CurNode].m_pFrames[pData->m_Textures[pData->m_CurNode].m_nCurRemap];
                    }
                    return material;
                }, data);
            }
            return atomic;
        }, &data);
    }
}