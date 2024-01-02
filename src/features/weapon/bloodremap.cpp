#include "pch.h"
#include "bloodremap.h"
#include <TxdDef.h>
#include <CTxdStore.h>
#include <CKeyGen.h>
#include <CStats.h>

BloodRemapFeature BloodRemap;

static ThiscallEvent <AddressList<0x63075C, H_CALL>, PRIORITY_BEFORE, ArgPickN<CPed*, 0>, void(CPed*)> pedKilledEvent;

void BloodRemapFeature::Initialize(RwFrame* frame, CWeapon* pWeapon) {
    pedKilledEvent += [&](CPed* pPed) {
        TextureData &data = xData.Get(&pPed->m_aWeapons[pPed->m_nActiveWeaponSlot]);
        data.m_nKills++;

        if (data.m_nKills > 3) data.m_nKills = 3;
    };
}

void BloodRemapFeature::Process(RwFrame* frame, CWeapon *pWeapon) {
    TextureData &data = xData.Get(pWeapon);

    if (!data.m_bInit) {
        Initialize(frame, pWeapon);
        data.m_bInit = true;
    }

    CWeaponInfo* pWeaponInfo = CWeaponInfo::GetWeaponInfo(pWeapon->m_eWeaponType, FindPlayerPed()->GetWeaponSkill(pWeapon->m_eWeaponType));
    if (!pWeaponInfo) return;

    CWeaponModelInfo* pWeaponModelInfo = static_cast<CWeaponModelInfo*>(CModelInfo::GetModelInfo(pWeaponInfo->m_nModelId1));
    if (!pWeaponModelInfo) return;

    TxdDef pedTxd = CTxdStore::ms_pTxdPool->m_pObjects[pWeaponModelInfo->m_nTxdIndex];
    RwTexDictionary * pedTxdDic = pedTxd.m_pRwDictionary;
    if (!pedTxdDic) return;

    std::string key = "";
    if (data.m_nKills == 0) key = "chainsaw_1bld";
    if (data.m_nKills == 1) key = "chainsaw_1bld_1";
    if (data.m_nKills == 2) key = "chainsaw_1bld_2";
    if (data.m_nKills == 3) key = "chainsaw_1bld_3";

    RwTexture *pTexture = RwTexDictionaryFindHashNamedTexture(pedTxdDic, CKeyGen::GetUppercaseKey(key.c_str()));
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