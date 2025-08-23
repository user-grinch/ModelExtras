#include "pch.h"
#include "dirtfx.h"
#include "RenderWare.h"
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include "texmgr.h"

void DirtFx::Initialize()
{
	m_bEnabled = true;
	patch::Nop(0x6D0E7E, 5);
    patch::Nop(0x4C9648, 5);
	patch::RedirectCall(0x53CA75, DirtFx::Shutdown);
	patch::RedirectCall(0x53CA61, DirtFx::Shutdown);
	patch::RedirectCall(0x5B8FFD, DirtFx::InitialiseDirtTextures);

	plugin::Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model)
	{
		CVehicleModelInfo *pInfo = (CVehicleModelInfo *)CModelInfo::GetModelInfo(model);
		CTxdStore::PushCurrentTxd();
		CTxdStore::SetCurrentTxd(pInfo->m_nTxdIndex);
		RwTexDictionaryForAllTextures(RwTexDictionaryGetCurrent(), [](RwTexture *texture, void *pData)
		{ 
			RwTexture *pDirtTex = texture;
			std::string dirtName = pDirtTex->name;

			if (dirtName.starts_with("#") || dirtName.starts_with("remap") || !dirtName.ends_with("_dt")) {
                return texture;
            }

			std::string cleanName = dirtName.substr(0, dirtName.find_last_of('_'));
			RwTexture *pCleanTex = TextureMgr::FindInDict(cleanName, RwTexDictionaryGetCurrent());

			if (pCleanTex) {
				InitialiseBlendTextureSingleEx(pCleanTex, pDirtTex);
			}
			return texture; 
		}, NULL);
		CTxdStore::PopCurrentTxd();
	};
}

void DirtFx::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat) {
	if (!m_bEnabled) {
		return;
	}
	
	std::string texName = pMat->texture->name;
	int dirtLvl = pVeh->m_fDirtLevel;

	if (texName == "vehiclegrunge256")
        RpMaterialSetTexture(pMat, ms_aDirtTextures[dirtLvl]);
	else if (texName == "vehicle_genericmud_truck" || texName == "vehiclegrunge_iv")
		RpMaterialSetTexture(pMat, ms_aDirtTextures_2[dirtLvl]);
	else if (texName == "vehiclegrunge512")
		RpMaterialSetTexture(pMat, ms_aDirtTextures_3[dirtLvl]);
	else if (texName.starts_with("tyrewall_dirt"))
		RpMaterialSetTexture(pMat, ms_aDirtTextures_4[dirtLvl]);
	else
	{
		if (m_DirtTextures.contains(texName))
		{
			RpMaterialSetTexture(pMat, m_DirtTextures[texName][dirtLvl]);
		}
	}
}

void DirtFx::InitialiseDirtTexture() // org
{
	((void(__cdecl *)())0x5D5BC0)();
}

void DirtFx::Shutdown()
{
	((void(__cdecl *)())0x5D5AD0)(); // Shutdown
	for (int i = 0; i < 16; i++)
	{
		if (ms_aDirtTextures_2[i]) RwTextureDestroy(ms_aDirtTextures_2[i]);
		if (ms_aDirtTextures_3[i]) RwTextureDestroy(ms_aDirtTextures_3[i]); 
		if (ms_aDirtTextures_4[i]) RwTextureDestroy(ms_aDirtTextures_4[i]);		// RwTextureDestroy(ms_aDirtTextures_5[i]);
	}
}

void DirtFx::InitialiseBlendTextureSingleEx(RwTexture *src, RwTexture *dest)
{
	if (src && dest && !m_DirtTextures.contains(src->name))
	{
		src->filterAddressing = rwFILTERLINEAR;
		dest->filterAddressing = rwFILTERLINEAR;
		for (size_t i = 0; i < 16; i++)
		{
			float FacB = (1.0 / 15.0) * i;
			float FacA = 1 - FacB;
			RwTexture *pTex = CClothesBuilder::CopyTexture(src);
			RwTextureSetName(pTex, src->name);
			CClothesBuilder::BlendTextures(pTex, dest, FacA, FacB);
			m_DirtTextures[src->name].push_back(pTex);
		}
	}
}

void DirtFx::InitialiseBlendTextureSingle(const char *CleanName, const char *DirtName, RwTexture **TextureArray)
{
	RwTexture *SrcTexture;
	RwTexture *DestTexture;

	SrcTexture = TextureMgr::Get(CleanName);
	if (!SrcTexture) {
		return;
	}
	SrcTexture->filterAddressing = rwFILTERLINEAR;

	DestTexture = TextureMgr::Get(DirtName);
	if (!DestTexture) {
		return;
	}
	DestTexture->filterAddressing = rwFILTERLINEAR;

	if (SrcTexture && DestTexture)
	{
		for (size_t i = 0; i < 16; i++)
		{
			float FacB = (1.0 / 15.0) * i;
			float FacA = 1 - FacB;
			TextureArray[i] = CClothesBuilder::CopyTexture(SrcTexture);
			RwTextureSetName(TextureArray[i], CleanName);
			CClothesBuilder::BlendTextures(TextureArray[i], DestTexture, FacA, FacB);
		}
	}
}

void DirtFx::InitialiseDirtTextureSingle(const char *name, RwTexture **dirtTextureArray)
{
	RwTexture *pTex = TextureMgr::Get(name);
	if (!pTex)
	{
		return;
	}
	pTex->filterAddressing = rwFILTERLINEAR;

	for (int texid = 0; texid < 16; texid++)
	{
		dirtTextureArray[texid] = CClothesBuilder::CopyTexture(pTex);
		RwTextureSetName(dirtTextureArray[texid], name);
		int alpha = 255 - texid * 15;
		RwRaster *dirtRaster = dirtTextureArray[texid]->raster;
		RwUInt8 *pixelsRaw = RwRasterLock(dirtRaster, 0, rwRASTERLOCKWRITE);
		if (!pixelsRaw)
		{
			return;
		}

		const int width = pTex->raster->width;
		const int height = pTex->raster->height;
		RwRGBA *pixels = reinterpret_cast<RwRGBA *>(pixelsRaw);

		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				RwRGBA &pixel = pixels[y * width + x];
				pixel.alpha = alpha;
			}
		}
		RwRasterUnlock(dirtRaster);
	}
}

void DirtFx::InitialiseDirtTextures()
{
	InitialiseDirtTexture();

	// Dirt Textures which blend to white
	InitialiseDirtTextureSingle("vehiclegrunge_iv", ms_aDirtTextures_2);
	InitialiseDirtTextureSingle("vehiclegrunge512", ms_aDirtTextures_3);

	// Textures which belnd between two images
	InitialiseBlendTextureSingle("tyrewall_dirt", "tyrewall_dirt_dt", ms_aDirtTextures_4);
	// InitialiseBlendTextureSingle((char *)"vehicle_generic_detail", (char *)"vehicle_generic_detaild", ms_aDirtTextures_5);
}