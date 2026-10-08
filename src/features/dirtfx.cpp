#include "pch.h"
#include "dirtfx.h"
#include "utils/texmgr.h"
#include "utils/modelinfomgr.h"
#include "utils/frameextension.h"
#include <d3d9.h>
#include <memory>
#include <cmath>

using namespace plugin;

static CdeclEvent<AddressList<0x53CA75, H_CALL, 0x53CA61, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> CCarFXRenderer__ShutdownEvent;
static CdeclEvent<AddressList<0x5B8FFD, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> CCarFXRenderer__InitialiseDirtTextureEvent;

using DirtImage = std::unique_ptr<RwImage, decltype(&RwImageDestroy)>;

// SA's RwImageSetFromRaster rejects compressed rasters. Read their native
// D3D9 format (not cFormat, which describes the pixel layout).
static bool ReadDirtImage(RwTexture *texture, DirtImage &image)
{
	if (!texture || !texture->raster) return false;
	RwRaster *raster = texture->raster;
	if (raster->width <= 0 || raster->height <= 0) return false;
	if (!image)
	{
		image.reset(RwImageCreate(raster->width, raster->height, 32));
		if (!image || !RwImageAllocatePixels(image.get())) return false;
	}
	if (image->width != raster->width || image->height != raster->height) return false;
	if (!RwD3D9RasterIsCompressed(raster))
	{
		return RwImageSetFromRaster(image.get(), raster) != nullptr;
	}

	// The D3D9 device raster plugin starts with its native texture pointer.
	int offset = RwRasterGetPluginOffset(rwID_DEVICEMODULE);
	if (offset < 0) return false;
	auto *native = *reinterpret_cast<IDirect3DBaseTexture9 **>(reinterpret_cast<char *>(raster) + offset);
	if (!native || native->GetType() != D3DRTYPE_TEXTURE) return false;
	auto *d3dTexture = static_cast<IDirect3DTexture9 *>(native);
	D3DSURFACE_DESC desc{};
	if (FAILED(d3dTexture->GetLevelDesc(0, &desc)) ||
		desc.Width != static_cast<UINT>(raster->width) || desc.Height != static_cast<UINT>(raster->height) ||
		(desc.Format != D3DFMT_DXT1 && desc.Format != D3DFMT_DXT3 && desc.Format != D3DFMT_DXT5))
		return false;
	D3DLOCKED_RECT locked{};
	if (FAILED(d3dTexture->LockRect(0, &locked, nullptr, D3DLOCK_READONLY))) return false;
	const int blockSize = desc.Format == D3DFMT_DXT1 ? 8 : 16;
	if (!locked.pBits || locked.Pitch < ((raster->width + 3) / 4) * blockSize)
	{
		d3dTexture->UnlockRect(0);
		return false;
	}
	for (int by = 0; by < raster->height; by += 4)
	{
		for (int bx = 0; bx < raster->width; bx += 4)
		{
			const auto *block = static_cast<const RwUInt8 *>(locked.pBits) + (by / 4) * locked.Pitch + (bx / 4) * blockSize;
			const auto *colorBlock = block + blockSize - 8;
			RwUInt16 endpoints[2];
			RwUInt32 indices;
			std::memcpy(endpoints, colorBlock, sizeof(endpoints));
			std::memcpy(&indices, colorBlock + 4, sizeof(indices));
			RwRGBA colors[4]{};
			for (int c = 0; c < 2; ++c)
			{
				const int r = endpoints[c] >> 11, g = (endpoints[c] >> 5) & 63, b = endpoints[c] & 31;
				colors[c] = {static_cast<RwUInt8>((r << 3) | (r >> 2)), static_cast<RwUInt8>((g << 2) | (g >> 4)),
					static_cast<RwUInt8>((b << 3) | (b >> 2)), 255};
			}
			const bool fourColors = desc.Format != D3DFMT_DXT1 || endpoints[0] > endpoints[1];
			for (int c = 2; c < (fourColors ? 4 : 3); ++c)
			{
				const int a = fourColors ? 4 - c : 1, b = fourColors ? c - 1 : 1, divisor = a + b;
				colors[c] = {static_cast<RwUInt8>((a * colors[0].red + b * colors[1].red) / divisor),
					static_cast<RwUInt8>((a * colors[0].green + b * colors[1].green) / divisor),
					static_cast<RwUInt8>((a * colors[0].blue + b * colors[1].blue) / divisor), 255};
			}
			RwUInt8 alphas[8] = {block[0], block[1], 0, 0, 0, 0, 0, 255};
			uint64_t alphaBits = 0;
			if (desc.Format == D3DFMT_DXT5)
			{
				const int divisor = alphas[0] > alphas[1] ? 7 : 5;
				for (int a = 1; a < divisor; ++a)
					alphas[a + 1] = static_cast<RwUInt8>(((divisor - a) * alphas[0] + a * alphas[1]) / divisor);
				for (int a = 0; a < 6; ++a) alphaBits |= static_cast<uint64_t>(block[a + 2]) << (8 * a);
			}
			else if (desc.Format == D3DFMT_DXT3) std::memcpy(&alphaBits, block, sizeof(alphaBits));
			for (int y = 0; y < 4 && by + y < raster->height; ++y)
			{
				auto *row = reinterpret_cast<RwRGBA *>(image->cpPixels + (by + y) * image->stride);
				for (int x = 0; x < 4 && bx + x < raster->width; ++x)
				{
					const int pixel = y * 4 + x;
					RwRGBA color = colors[(indices >> (2 * pixel)) & 3];
					if (desc.Format == D3DFMT_DXT3) color.alpha = static_cast<RwUInt8>(((alphaBits >> (4 * pixel)) & 15) * 17);
					if (desc.Format == D3DFMT_DXT5) color.alpha = alphas[(alphaBits >> (3 * pixel)) & 7];
					row[bx + x] = color;
				}
			}
		}
	}
	d3dTexture->UnlockRect(0);
	return true;
}

static RwTexture *FindDirtTexture(RwTexture *clean, bool &overlay)
{
	if (!clean || !clean->dict || !clean->name) return nullptr;
	char name[32];
	size_t length = strnlen(clean->name, sizeof(name));
	// Keep the existing _dt replacement convention ahead of IVF alpha overlays.
	for (int pass = 0; pass < 2; ++pass)
	{
		for (const char *suffix : {"_dt", "_dirt", "_d"})
		{
			const size_t suffixLength = std::strlen(suffix);
			if (length + suffixLength >= sizeof(name) || (pass && std::strcmp(suffix, "_dt") == 0)) continue;
			std::memcpy(name, clean->name, length);
			std::memcpy(name + length, suffix, suffixLength + 1);
			RwTexture *dirt = RwTexDictionaryFindNamedTexture(clean->dict, name);
			if (!dirt && _strnicmp(name, "#emap", 5) == 0)
			{
				name[0] = 'r'; // SA marks vanilla paintjob textures with '#'.
				dirt = RwTexDictionaryFindNamedTexture(clean->dict, name);
			}
			if (dirt)
			{
				overlay = std::strcmp(suffix, "_dt") != 0;
				return dirt;
			}
		}
		// A random remap may share its base's IVF overlay. A complete _dt image
		// cannot be shared: that would replace the selected livery at full dirt.
		size_t remap = 0;
		while (remap + 6 <= length && _strnicmp(clean->name + remap, "_remap", 6) != 0) ++remap;
		if (remap == 0 || remap + 6 > length || clean->name[0] == '#') break;
		length = remap;
	}
	return nullptr;
}

void DirtFx::RegisterVehicleTextures(int model)
{
	if (!m_bEnabled || !m_bCustomReady) return;
	auto *info = CModelInfo::GetModelInfo(model);
	if (!info || info->m_nTxdIndex < 0 || !CTxdStore::ms_pTxdPool) return;
	auto *def = CTxdStore::ms_pTxdPool->GetAt(info->m_nTxdIndex);
	if (!def || !def->m_pRwDictionary) return;
	RwTexDictionaryForAllTextures(def->m_pRwDictionary, [](RwTexture *texture, void *)
	{
		if (!texture || m_DirtTextures.contains(texture)) return texture;
		bool overlay = false;
		if (RwTexture *dirt = FindDirtTexture(texture, overlay))
		{
			InitialiseBlendTextureSingleEx(texture, dirt, overlay);
		}
		return texture;
	}, nullptr);
}

void DirtFx::Reload(CVehicle *vehicle)
{
	ReloadConfig();
	if (vehicle) RegisterVehicleTextures(vehicle->m_nModelIndex);
}

void DirtFx::ReloadConfig()
{
	CBaseFeature::ReloadConfig();
	m_bEnabled = m_bActive;
}

void DirtFx::Init()
{
	ReloadConfig();
	Events::attachRwPluginsEvent += []()
	{
		// Release cached copies when either source streams out; pointer reuse must
		// never give another model's textures an old dirt cache.
		m_bCustomReady = RwTextureRegisterPlugin(0, PLUGIN_ID_NUM, nullptr,
			[](void *object, RwInt32, RwInt32) -> void *
			{
				auto *texture = static_cast<RwTexture *>(object);
				auto it = m_DirtTextures.find(texture);
				if (it != m_DirtTextures.end())
				{
					for (int level = 1; level < 16; ++level)
					{
						if (it->second.textures[level])
						{
							RwTextureDestroy(it->second.textures[level]);
							it->second.textures[level] = nullptr;
						}
					}
					m_DirtTextures.erase(it);
				}
				return object;
			}, nullptr) >= 0;
	};
	
	if (injector::GetBranchDestination(0x6D0E7E).as_int() != 0x5D5DB0) LOG(ERROR) << "Address conflict on 0x6D0E7E";
	patch::Nop(0x6D0E7E, 5);

	// revert & remove hook from silentpatch
	patch::SetRaw(0x4C9648, (void *)"\x88\x86\xD2\x02\x00", 5);
	
	CCarFXRenderer__ShutdownEvent.after += DirtFx::ShutdownHook;
	CCarFXRenderer__InitialiseDirtTextureEvent.after += DirtFx::InitialiseDirtTextures;

	Events::vehicleSetModelEvent.after += [](CVehicle *vehicle, int model)
	{
		if (vehicle) RegisterVehicleTextures(model);
	};
}

void DirtFx::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat, RwTexture *baseTexture) {
	if (!m_bEnabled || !pVeh || !pMat || !pMat->texture) {
		return;
	}
	
	const char *rawName = pMat->texture->name;
	if (!rawName) return;

	char first = rawName[0];
	const float dirtLevel = std::isfinite(pVeh->m_fDirtLevel) ? pVeh->m_fDirtLevel : 0.0f;
	int dirtLvl = static_cast<int>(std::clamp(dirtLevel, 0.0f, 15.0f));
	RwTexture *target = nullptr;

	if (first == 'v') {
		std::string_view texName(rawName);
		if (texName == "vehiclegrunge256") {
			target = ms_aDirtTextures[dirtLvl];
		}
		else if (texName == "vehicle_genericmud_truck" || texName == "vehiclegrunge_iv") {
			target = ms_aDirtTextures_2[dirtLvl];
		}
		else if (texName == "vehiclegrunge512") {
			target = ms_aDirtTextures_3[dirtLvl];
		}
	} else if (first == 't') {
		std::string_view texName(rawName);
		if (texName.starts_with("tyrewall_dirt")) {
			target = ms_aDirtTextures_4[dirtLvl];
		}
	}

	if (!target && dirtLvl > 0)
	{
		auto it = m_DirtTextures.find(pMat->texture);
		if (it != m_DirtTextures.end())
		{
			target = it->second.textures[dirtLvl];
		}
		else
		{
			bool overlay = false;
			RwTexture *dirt = FindDirtTexture(pMat->texture, overlay);
			if (!dirt && baseTexture) dirt = FindDirtTexture(baseTexture, overlay);
			if (dirt)
			{
				InitialiseBlendTextureSingleEx(pMat->texture, dirt, overlay);
				auto newIt = m_DirtTextures.find(pMat->texture);
				if (newIt != m_DirtTextures.end())
				{
					target = newIt->second.textures[dirtLvl];
				}
			}
			else
			{
				m_DirtTextures[pMat->texture] = DirtStages{.initialized = true};
			}
		}
	}

	if (target && target != pMat->texture)
	{
		ModelInfoMgr::RegisterRestore(&pMat->texture, pMat->texture);
		pMat->texture = target;
	}
}

void DirtFx::Shutdown()
{
	plugin::Call<0x5D5AD0>(); // CCarFXRenderer__Shutdown
	ShutdownHook();
}

void DirtFx::ShutdownHook()
{
	for (auto &[tex, stages] : m_DirtTextures)
	{
		for (int level = 1; level < 16; ++level)
		{
			if (stages.textures[level])
			{
				RwTextureDestroy(stages.textures[level]);
				stages.textures[level] = nullptr;
			}
		}
	}
	m_DirtTextures.clear();
	for (int i = 0; i < 16; i++)
	{
		if (ms_aDirtTextures_2[i]) { RwTextureDestroy(ms_aDirtTextures_2[i]); ms_aDirtTextures_2[i] = nullptr; }
		if (ms_aDirtTextures_3[i]) { RwTextureDestroy(ms_aDirtTextures_3[i]); ms_aDirtTextures_3[i] = nullptr; }
		if (ms_aDirtTextures_4[i]) { RwTextureDestroy(ms_aDirtTextures_4[i]); ms_aDirtTextures_4[i] = nullptr; }
	}
}

void DirtFx::InitialiseBlendTextureSingleEx(RwTexture *src, RwTexture *dest, bool overlay)
{
	if (!m_bCustomReady || !src || !dest || !src->raster || !dest->raster) return;
	if (src->raster->width <= 0 || src->raster->height <= 0 || dest->raster->width <= 0 || dest->raster->height <= 0) return;

	auto it = m_DirtTextures.find(src);
	if (it != m_DirtTextures.end() && it->second.initialized) return;

	DirtImage cleanImage{nullptr, RwImageDestroy};
	DirtImage dirtImage{nullptr, RwImageDestroy};

	if (!ReadDirtImage(src, cleanImage) || !ReadDirtImage(dest, dirtImage) || !cleanImage || !dirtImage)
	{
		m_DirtTextures[src] = DirtStages{.initialized = true};
		return;
	}

	const int width = overlay ? std::max(cleanImage->width, dirtImage->width) : cleanImage->width;
	const int height = overlay ? std::max(cleanImage->height, dirtImage->height) : cleanImage->height;
	if (width <= 0 || height <= 0) return;

	DirtStages stages;
	stages.initialized = true;

	const int flags = rwRASTERTYPETEXTURE | (RwRasterGetFormat(src->raster) & rwRASTERFORMATMIPMAP);

	DirtImage blendedImage{RwImageCreate(width, height, 32), RwImageDestroy};
	if (!blendedImage || !RwImageAllocatePixels(blendedImage.get()))
	{
		m_DirtTextures[src] = stages;
		return;
	}

	for (int level = 1; level < 16; ++level)
	{
		const float factor = static_cast<float>(level) / 15.0f;
		bool hasAlpha = false;

		for (int y = 0; y < height; ++y)
		{
			const auto *cleanRow = reinterpret_cast<const RwRGBA *>(cleanImage->cpPixels + (static_cast<size_t>(y) * cleanImage->height / height) * cleanImage->stride);
			const auto *dirtRow = reinterpret_cast<const RwRGBA *>(dirtImage->cpPixels + (static_cast<size_t>(y) * dirtImage->height / height) * dirtImage->stride);
			auto *out = reinterpret_cast<RwRGBA *>(blendedImage->cpPixels + y * blendedImage->stride);

			for (int x = 0; x < width; ++x)
			{
				const RwRGBA &a = cleanRow[static_cast<size_t>(x) * cleanImage->width / width];
				const RwRGBA &b = dirtRow[static_cast<size_t>(x) * dirtImage->width / width];
				const float weight = overlay ? factor * b.alpha / 255.0f : factor;

				out[x] = {
					static_cast<RwUInt8>(a.red * (1.0f - weight) + b.red * weight),
					static_cast<RwUInt8>(a.green * (1.0f - weight) + b.green * weight),
					static_cast<RwUInt8>(a.blue * (1.0f - weight) + b.blue * weight),
					overlay ? a.alpha : static_cast<RwUInt8>(a.alpha * (1.0f - factor) + b.alpha * factor)
				};
				hasAlpha |= (out[x].alpha != 255);
			}
		}

		RwRaster *raster = RwRasterCreate(width, height, hasAlpha ? 32 : 24,
			flags | (hasAlpha ? rwRASTERFORMAT8888 : rwRASTERFORMAT888));
		if (raster)
		{
			if (RwRasterSetFromImage(raster, blendedImage.get()) &&
				(!(flags & rwRASTERFORMATMIPMAP) || RwTextureRasterGenerateMipmaps(raster, blendedImage.get())))
			{
				RwTexture *texture = RwTextureCreate(raster);
				if (texture)
				{
					RwTextureSetName(texture, src->name);
					texture->filterAddressing = src->filterAddressing;
					stages.textures[level] = texture;
				}
				else
				{
					RwRasterDestroy(raster);
				}
			}
			else
			{
				RwRasterDestroy(raster);
			}
		}
	}

	m_DirtTextures[src] = stages;
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
			float FacB = (1.0f / 15.0f) * static_cast<float>(i);
			float FacA = 1.0f - FacB;
			TextureArray[i] = CClothesBuilder::CopyTexture(SrcTexture);
			RwTextureSetName(TextureArray[i], CleanName);
			CClothesBuilder::BlendTextures(TextureArray[i], DestTexture, FacA, FacB);
		}
	}
}

void DirtFx::InitialiseDirtTextureSingle(const char *name, RwTexture **dirtTextureArray)
{
	RwTexture *pTex = TextureMgr::Get(name);
	if (!pTex || !pTex->raster)
	{
		return;
	}
	pTex->filterAddressing = rwFILTERLINEAR;

	const int width = pTex->raster->width;
	const int height = pTex->raster->height;

	for (int texid = 0; texid < 16; texid++)
	{
		dirtTextureArray[texid] = CClothesBuilder::CopyTexture(pTex);
		if (!dirtTextureArray[texid])
		{
			continue;
		}

		RwTextureSetName(dirtTextureArray[texid], name);
		dirtTextureArray[texid]->filterAddressing = rwFILTERLINEAR;

		float factor = static_cast<float>(texid) / 15.0f;
		RwRaster *dirtRaster = dirtTextureArray[texid]->raster;
		RwUInt8 *pixelsRaw = RwRasterLock(dirtRaster, 0, rwRASTERLOCKWRITE);
		if (!pixelsRaw)
		{
			continue;
		}

		RwRGBA *pixels = reinterpret_cast<RwRGBA *>(pixelsRaw);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				RwRGBA &pixel = pixels[y * width + x];
				pixel.red   = static_cast<RwUInt8>(255 - static_cast<int>((255 - pixel.red)   * factor));
				pixel.green = static_cast<RwUInt8>(255 - static_cast<int>((255 - pixel.green) * factor));
				pixel.blue  = static_cast<RwUInt8>(255 - static_cast<int>((255 - pixel.blue)  * factor));
				pixel.alpha = 255;
			}
		}
		RwRasterUnlock(dirtRaster);
	}
}

void DirtFx::InitialiseDirtTextures()
{
	// Dirt Textures which blend to white
	InitialiseDirtTextureSingle("vehiclegrunge_iv", ms_aDirtTextures_2);
	InitialiseDirtTextureSingle("vehiclegrunge512", ms_aDirtTextures_3);

	// Textures which belnd between two images
	InitialiseBlendTextureSingle("tyrewall_dirt", "tyrewall_dirt_dt", ms_aDirtTextures_4);
}
