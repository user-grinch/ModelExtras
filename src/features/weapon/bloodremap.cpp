#include "pch.h"
#include "bloodremap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include <CKeyGen.h>
#include <CStats.h>

BloodRemapFeature BloodRemap;

void BloodRemapFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
    TextureData &data = texData.Get(pWeapon);

    CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(pWeapon->m_eWeaponType, FindPlayerPed()->GetWeaponSkill(pWeapon->m_eWeaponType));
    if (!pWeaponInfo) return;

    CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
    if (!pWeaponModelInfo) return;

    TxdDef pedTxd = CTxdStore::ms_pTxdPool->m_pObjects[pWeaponModelInfo->m_nTxdIndex];
    RwTexDictionary * pedTxdDic = pedTxd.m_pRwDictionary;
    if (!pedTxdDic) return;

    RwTexture *pTexture = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey("blood_none"));
    RpClumpForAllAtomics(pWeaponModelInfo->m_pRwClump, [](RpAtomic *atomic, void *data)
    {
        if (atomic->geometry)
        {
            RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial *material, void *data)
            {
                material->texture = reinterpret_cast<RwTexture*>(data);
                return material;
            }, data);
        }
        return atomic;
    }, pTexture);
}