#include "pch.h"
#include "fxrender.h"
#include "RenderWare.h"
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include "texmgr.h"

void FxRender::Initialize()
{
	patch::RedirectCall(0x53CA75, FxRender::Shutdown);
	patch::RedirectCall(0x53CA61, FxRender::Shutdown);
	patch::RedirectCall(0x5B8FFD, FxRender::InitialiseDirtTextures);
}

void FxRender::InitialiseDirtTexture() // org
{
	((void(__cdecl *)())0x5D5BC0)();
}

void FxRender::Shutdown()
{
	((void(__cdecl *)())0x5D5AD0)(); // Shutdown
	for (int i = 0; i < 16; i++)
	{
		RwTextureDestroy(ms_aDirtTextures_2[i]);
		RwTextureDestroy(ms_aDirtTextures_3[i]);
		RwTextureDestroy(ms_aDirtTextures_4[i]);
		// RwTextureDestroy(ms_aDirtTextures_5[i]);
		// RwTextureDestroy(ms_aDirtTextures_6[i]);
	}
}

void FxRender::InitialiseBlendTextureSingle(const char *CleanName, const char *DirtName, RwTexture **TextureArray)
{
	RwTexture *SrcTexture;
	RwTexture *DestTexture;

	SrcTexture = TextureMgr::Get(CleanName);
	SrcTexture->filterAddressing = rwFILTERLINEAR;

	DestTexture = TextureMgr::Get(DirtName);
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

void FxRender::InitialiseDirtTextureSingle(const char *name, RwTexture **dirtTextureArray)
{
	RwTexture *pTex = TextureMgr::Get(name);
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

void FxRender::InitialiseDirtTextures()
{
	InitialiseDirtTexture();

	// Dirt Textures which blend to white
	InitialiseDirtTextureSingle("vehiclegrunge_iv", ms_aDirtTextures_2);
	InitialiseDirtTextureSingle("vehiclegrunge512", ms_aDirtTextures_3);

	// Textures which belnd between two images
	// InitialiseBlendTextureSingle("tyrewall_dirt", "tyrewall_dirt_d", ms_aDirtTextures_4);
	// InitialiseBlendTextureSingle((char *)"vehicle_generic_detail", (char *)"vehicle_generic_detaild", ms_aDirtTextures_5);
}