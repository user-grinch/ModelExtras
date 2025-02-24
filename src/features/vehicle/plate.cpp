#include "pch.h"
#include "plate.h"
#include <CCustomCarPlateMgr.h>
#include <CWeather.h>
#include <rwcore.h>
#include <rpworld.h>

LicensePlateFeature LicensePlate;
extern bool IsNightTime();

void LicensePlateFeature::Initialize() {
    if (pCharSetTex) {
        return;
    }

    pCharSetTex = Util::LoadTextureFromFile(MOD_DATA_PATH_S(std::string("plates/platecharset.png")), 255);
    RwTextureSetFilterMode(pCharSetTex, rwFILTERNEAREST);
    RwTextureSetAddressingU(pCharSetTex, rwFILTERMIPNEAREST);
    RwTextureSetAddressingV(pCharSetTex, rwFILTERMIPNEAREST);


    // Don't add .dds extension!
    m_Plates[DAY_LS] = RwD3D9DDSTextureRead(MOD_DATA_PATH_S(std::string("plates/plateback3")), NULL);
    m_Plates[DAY_SF] = RwD3D9DDSTextureRead(MOD_DATA_PATH_S(std::string("plates/plateback1")), NULL);
    m_Plates[DAY_LV] = RwD3D9DDSTextureRead(MOD_DATA_PATH_S(std::string("plates/plateback2")), NULL);

    m_Plates[NIGHT_LS] = RwD3D9DDSTextureRead(MOD_DATA_PATH_S(std::string("plates/plateback3_l")), NULL);
    m_Plates[NIGHT_SF] = RwD3D9DDSTextureRead(MOD_DATA_PATH_S(std::string("plates/plateback1_l")), NULL);
    m_Plates[NIGHT_LV] = RwD3D9DDSTextureRead(MOD_DATA_PATH_S(std::string("plates/plateback2_l")), NULL);

    for (int i = 0; i < ePlateType::TOTAL_SZ; i++) {
        RwTextureSetAddressingU(pCharSetTex, rwFILTERMIPNEAREST);
        RwTextureSetAddressingV(pCharSetTex, rwFILTERMIPNEAREST);
        RwTextureSetFilterMode(pCharSetTex, rwFILTERLINEAR);
    }

    // plugin::patch::Nop(0x6FDF44, 4);
    plugin::patch::SetChar(0x6FDF47, rwFILTERLINEARMIPLINEAR);
    plugin::patch::ReplaceFunction(0x6FD500, CCustomCarPlateMgr_Initialise);
    plugin::patch::ReplaceFunction(0x6FDE50, CCustomCarPlateMgr_SetupMaterialPlatebackTexture);
}

RpMaterial* __cdecl LicensePlateFeature::CCustomCarPlateMgr_SetupMaterialPlatebackTexture(RpMaterial* material, char plateType) {
    if (plateType == -1)
    {
        if (CWeather::WeatherRegion == 1)
            plateType = 2;
        else
            plateType = CWeather::WeatherRegion > 2 && CWeather::WeatherRegion <= 4;
    }

    if (IsNightTime()) {
        material->surfaceProps.ambient = 4.0;
        RpMaterialSetTexture(material, m_Plates[plateType + 3]);
    }
    else {
        material->surfaceProps.ambient = 1.0;
        RpMaterialSetTexture(material, m_Plates[plateType]);
    }
    return material;
}



bool __cdecl LicensePlateFeature::CCustomCarPlateMgr_Initialise() {
    LicensePlate.Initialize();
    plugin::patch::SetPointer(0xC3EF5C, LicensePlate.pCharSetTex);
    plugin::patch::SetPointer(0xC3EF60, LicensePlate.m_Plates[DAY_SF]);
    plugin::patch::SetPointer(0xC3EF64, LicensePlate.m_Plates[DAY_LV]);
    plugin::patch::SetPointer(0xC3EF68, LicensePlate.m_Plates[DAY_LS]);
    plugin::patch::SetPointer(0xC3EF78, RwRasterLock(RwTextureGetRaster(LicensePlate.pCharSetTex), 0, rwRASTERLOCKREAD));
    return plugin::patch::GetPointer(0xC3EF78) != NULL;
}

void LicensePlateFeature::Process(RwFrame* frame, CVehicle* pVeh) {

}