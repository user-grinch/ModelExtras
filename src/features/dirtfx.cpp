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
static constexpr int DirtPixelsPerTick = 131072;

// SA's RwImageSetFromRaster rejects compressed rasters. Read their native
// D3D9 format (not cFormat, which describes the pixel layout) in small chunks.
static bool ReadDirtImage(RwTexture *texture, DirtImage &image, int &nextRow)
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
		if (!RwImageSetFromRaster(image.get(), raster)) return false;
		nextRow = raster->height;
		return true;
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
	const int lastRow = std::min(raster->height, nextRow + std::max(4, (DirtPixelsPerTick / raster->width) & ~3));
	for (int by = nextRow; by < lastRow; by += 4)
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
	nextRow = lastRow;
	return true;
}

static RwTexture *FindDirtTexture(RwTexture *clean, bool &overlay)
{
	if (!clean || !clean->dict) return nullptr;
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

// Only one preparation job owns temporary images; no GPU lock crosses ticks.
static struct DirtJob
{
	RwTexture *source = nullptr, *layer = nullptr;
	RwRaster *sourceRaster = nullptr, *layerRaster = nullptr;
	int level = 0, phase = 0, row = 0, width = 0, height = 0;
	int flags = 0;
	size_t bytes = 0;
	bool overlay = false, hasAlpha = false;
	DirtImage clean{nullptr, RwImageDestroy}, dirt{nullptr, RwImageDestroy}, blended{nullptr, RwImageDestroy};
} g_DirtJob;

static void CancelDirtJob()
{
	g_DirtJob = DirtJob{};
}

static size_t DirtRasterBytes(int width, int height, bool mipmaps)
{
	size_t bytes = 0;
	do
	{
		bytes += static_cast<size_t>(width) * height * 4; // D3D9 RGB rasters also use 32-bit storage.
		if (!mipmaps || (width == 1 && height == 1)) break;
		width = std::max(1, width / 2);
		height = std::max(1, height / 2);
	} while (true);
	return bytes;
}

void DirtFx::ReleaseDirtStage(DirtStages &stages, int level)
{
	if (RwTexture *texture = stages.textures[level])
	{
		stages.textures[level] = nullptr;
		m_nCustomCacheBytes -= stages.bytes[level];
		stages.bytes[level] = 0;
		RwTextureDestroy(texture);
	}
	stages.requested &= static_cast<RwUInt16>(~(1u << level));
	stages.attempted &= static_cast<RwUInt16>(~(1u << level));
	stages.lastUsed[level] = 0;
}

void DirtFx::DestroyDirtStages(DirtStages &stages)
{
	for (int level = 1; level < 16; ++level) ReleaseDirtStage(stages, level);
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
		bool overlay = false;
		if (RwTexture *dirt = FindDirtTexture(texture, overlay)) InitialiseBlendTextureSingleEx(texture, dirt, overlay);
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
				if (texture == g_DirtJob.source || texture == g_DirtJob.layer) CancelDirtJob();
				for (auto it = m_DirtTextures.begin(); it != m_DirtTextures.end();)
				{
					if (it->first == texture)
					{
						auto sources = std::move(it->second);
						it = m_DirtTextures.erase(it);
						for (auto &[source, stages] : sources) DestroyDirtStages(stages);
					}
					else
					{
						auto source = it->second.find(texture);
						if (source != it->second.end())
						{
							auto stages = source->second;
							it->second.erase(source);
							DestroyDirtStages(stages);
						}
						++it;
					}
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

	// Vanilla loads selected paintjobs later than the vehicle model. Register
	// their sources on the script tick, never inside the material render callback.
	Events::processScriptsEvent += []()
	{
		if (!m_bEnabled || !m_bCustomReady || !CPools::ms_pVehiclePool) return;
		for (CVehicle *vehicle : CPools::ms_pVehiclePool)
		{
			if (!vehicle || !vehicle->m_pRemapTexture) continue;
			bool overlay = false;
			if (RwTexture *dirt = FindDirtTexture(vehicle->m_pRemapTexture, overlay))
			{
				InitialiseBlendTextureSingleEx(vehicle->m_pRemapTexture, dirt, overlay);
				continue;
			}
			auto *info = CModelInfo::GetModelInfo(vehicle->m_nModelIndex);
			if (!info || info->m_nTxdIndex < 0 || !CTxdStore::ms_pTxdPool) continue;
			auto *def = CTxdStore::ms_pTxdPool->GetAt(info->m_nTxdIndex);
			if (!def || !def->m_pRwDictionary) continue;
			for (auto &[dirt, sources] : m_DirtTextures)
			{
				if (dirt->dict != def->m_pRwDictionary || !std::any_of(sources.begin(), sources.end(),
					[](const auto &source) { return source.first->name[0] == '#'; })) continue;
				const size_t length = std::strlen(dirt->name);
				overlay = length < 3 || _stricmp(dirt->name + length - 3, "_dt") != 0;
				if (overlay) InitialiseBlendTextureSingleEx(vehicle->m_pRemapTexture, dirt, true);
			}
		}
		// Finish a bounded part of one job per tick; render callbacks only request levels.
		if (g_DirtJob.source)
		{
			ProcessDirtJob();
			return;
		}
		for (auto &[dirt, sources] : m_DirtTextures)
		{
			for (auto &[source, stages] : sources)
			{
				for (int level = 1; level < 16; ++level)
				{
					if ((stages.requested & ~stages.attempted) & (1u << level))
					{
						InitialiseDirtStage(source, dirt, stages, level);
						return;
					}
				}
			}
		}
	};
}

void DirtFx::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat, RwTexture *baseTexture) {
	if (!m_bEnabled || !pVeh || !pMat || !pMat->texture) {
		return;
	}
	
	const char *rawName = pMat->texture->name;
	char first = rawName[0];
	const float dirtLevel = std::isfinite(pVeh->m_fDirtLevel) ? pVeh->m_fDirtLevel : 0.0f;
	int dirtLvl = static_cast<int>(std::clamp(dirtLevel, 0.0f, 15.0f));
	RwTexture *target = nullptr;

	if (first == 'v') {
		std::string_view texName(rawName);
		if (texName == "vehiclegrunge256") {
			target = ms_aDirtTextures[dirtLvl];
		}
		if (texName == "vehicle_genericmud_truck" || texName == "vehiclegrunge_iv") {
			target = ms_aDirtTextures_2[dirtLvl];
		}
		if (texName == "vehiclegrunge512") {
			target = ms_aDirtTextures_3[dirtLvl];
		}
	} else if (first == 't') {
		std::string_view texName(rawName);
		if (texName.starts_with("tyrewall_dirt")) {
			target = ms_aDirtTextures_4[dirtLvl];
		}
	}

	if (!target && dirtLvl > 0 && !m_DirtTextures.empty())
	{
		bool overlay = false;
		RwTexture *dirt = FindDirtTexture(pMat->texture, overlay);
		if (!dirt && baseTexture) dirt = FindDirtTexture(baseTexture, overlay);
		auto it = m_DirtTextures.find(dirt);
		if (it != m_DirtTextures.end())
		{
			auto source = it->second.find(pMat->texture);
			if (source != it->second.end())
			{
				source->second.requested |= static_cast<RwUInt16>(1u << dirtLvl);
				target = source->second.textures[dirtLvl];
				if (target) source->second.lastUsed[dirtLvl] = ++m_nTextureUse;
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
	CancelDirtJob();
	auto custom = std::move(m_DirtTextures);
	m_DirtTextures.clear();
	for (auto &[dirt, sources] : custom)
		for (auto &[source, stages] : sources) DestroyDirtStages(stages);
	for (int i = 0; i < 16; i++)
	{
		if (ms_aDirtTextures_2[i]) RwTextureDestroy(ms_aDirtTextures_2[i]);
		if (ms_aDirtTextures_3[i]) RwTextureDestroy(ms_aDirtTextures_3[i]); 
		if (ms_aDirtTextures_4[i]) RwTextureDestroy(ms_aDirtTextures_4[i]);		// RwTextureDestroy(ms_aDirtTextures_5[i]);
	}
}

void DirtFx::InitialiseBlendTextureSingleEx(RwTexture *src, RwTexture *dest, bool overlay)
{
	if (!m_bCustomReady || !src || !dest || !src->raster || !dest->raster) return;
	m_DirtTextures[dest].try_emplace(src).first->second.overlay = overlay;
}

void DirtFx::InitialiseDirtStage(RwTexture *src, RwTexture *dest, DirtStages &stages, int level)
{
	stages.attempted |= static_cast<RwUInt16>(1u << level);
	if (!src || !dest || !src->raster || !dest->raster) return;
	const int width = stages.overlay ? std::max(src->raster->width, dest->raster->width) : src->raster->width;
	const int height = stages.overlay ? std::max(src->raster->height, dest->raster->height) : src->raster->height;
	// Reject oversized/invalid inputs before allocating temporary images.
	for (RwTexture *texture : {src, dest})
		if (texture->raster->width <= 0 || texture->raster->height <= 0 ||
			static_cast<uint64_t>(texture->raster->width) * texture->raster->height * 4 > CustomCacheLimit) return;
	if (static_cast<uint64_t>(width) * height * 4 > CustomCacheLimit) return;
	const size_t bytes = DirtRasterBytes(width, height, (RwRasterGetFormat(src->raster) & rwRASTERFORMATMIPMAP) != 0);
	if (bytes > CustomCacheLimit) return;
	g_DirtJob.source = src;
	g_DirtJob.layer = dest;
	g_DirtJob.sourceRaster = src->raster;
	g_DirtJob.layerRaster = dest->raster;
	g_DirtJob.level = level;
	g_DirtJob.width = width;
	g_DirtJob.height = height;
	g_DirtJob.overlay = stages.overlay;
	g_DirtJob.flags = rwRASTERTYPETEXTURE | (RwRasterGetFormat(src->raster) & rwRASTERFORMATMIPMAP);
	g_DirtJob.bytes = bytes;
}

void DirtFx::ProcessDirtJob()
{
	auto &job = g_DirtJob;
	if (!job.source || !job.layer || job.source->raster != job.sourceRaster || job.layer->raster != job.layerRaster)
	{
		CancelDirtJob();
		return;
	}
	if (job.phase < 2)
	{
		DirtImage &image = job.phase == 0 ? job.clean : job.dirt;
		if (!ReadDirtImage(job.phase == 0 ? job.source : job.layer, image, job.row))
		{
			CancelDirtJob();
			return;
		}
		if (job.row == image->height) { ++job.phase; job.row = 0; }
		return;
	}
	const int width = job.overlay ? std::max(job.clean->width, job.dirt->width) : job.clean->width;
	const int height = job.overlay ? std::max(job.clean->height, job.dirt->height) : job.clean->height;
	if (width != job.width || height != job.height) { CancelDirtJob(); return; }
	if (job.phase == 2)
	{
		job.blended.reset(RwImageCreate(width, height, 32));
		if (!job.blended || !RwImageAllocatePixels(job.blended.get())) { CancelDirtJob(); return; }
		++job.phase;
		return;
	}
	if (job.phase == 3)
	{
		const float factor = static_cast<float>(job.level) / 15.0f;
		const int lastRow = std::min(height, job.row + std::max(1, DirtPixelsPerTick / width));
		for (int y = job.row; y < lastRow; ++y)
		{
			const auto *cleanRow = reinterpret_cast<const RwRGBA *>(job.clean->cpPixels + (static_cast<size_t>(y) * job.clean->height / height) * job.clean->stride);
			const auto *dirtRow = reinterpret_cast<const RwRGBA *>(job.dirt->cpPixels + (static_cast<size_t>(y) * job.dirt->height / height) * job.dirt->stride);
			auto *out = reinterpret_cast<RwRGBA *>(job.blended->cpPixels + y * job.blended->stride);
			for (int x = 0; x < width; ++x)
			{
				const RwRGBA &a = cleanRow[static_cast<size_t>(x) * job.clean->width / width], &b = dirtRow[static_cast<size_t>(x) * job.dirt->width / width];
				const float weight = job.overlay ? factor * b.alpha / 255.0f : factor;
				out[x] = {static_cast<RwUInt8>(a.red * (1.0f - weight) + b.red * weight),
					static_cast<RwUInt8>(a.green * (1.0f - weight) + b.green * weight),
					static_cast<RwUInt8>(a.blue * (1.0f - weight) + b.blue * weight),
					job.overlay ? a.alpha : static_cast<RwUInt8>(a.alpha * (1.0f - factor) + b.alpha * factor)};
				job.hasAlpha |= out[x].alpha != 255;
			}
		}
		job.row = lastRow;
		if (job.row == height) ++job.phase;
		return;
	}
	auto &stages = m_DirtTextures.at(job.layer).at(job.source);
	// Evict the least recently rendered copies before allocating the new raster.
	while (m_nCustomCacheBytes > CustomCacheLimit - job.bytes)
	{
		DirtStages *oldest = nullptr;
		int oldLevel = 0;
		uint64_t age = UINT64_MAX;
		for (auto &[dirt, sources] : m_DirtTextures)
			for (auto &[source, cached] : sources)
				for (int level = 1; level < 16; ++level)
					if (cached.textures[level] && cached.lastUsed[level] < age)
					{ oldest = &cached; oldLevel = level; age = cached.lastUsed[level]; }
		if (!oldest) { CancelDirtJob(); return; }
		ReleaseDirtStage(*oldest, oldLevel);
	}
	RwRaster *raster = RwRasterCreate(width, height, job.hasAlpha ? 32 : 24,
		job.flags | (job.hasAlpha ? rwRASTERFORMAT8888 : rwRASTERFORMAT888));
	RwTexture *texture = nullptr;
	if (raster && RwRasterSetFromImage(raster, job.blended.get()) &&
		(!(job.flags & rwRASTERFORMATMIPMAP) || RwTextureRasterGenerateMipmaps(raster, job.blended.get())))
		texture = RwTextureCreate(raster);
	if (texture)
	{
		RwTextureSetName(texture, job.source->name);
		texture->filterAddressing = job.source->filterAddressing;
		stages.textures[job.level] = texture;
		stages.lastUsed[job.level] = ++m_nTextureUse;
		stages.bytes[job.level] = job.bytes;
		m_nCustomCacheBytes += job.bytes;
	}
	else if (raster) RwRasterDestroy(raster);
	CancelDirtJob();
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
	// Dirt Textures which blend to white
	InitialiseDirtTextureSingle("vehiclegrunge_iv", ms_aDirtTextures_2);
	InitialiseDirtTextureSingle("vehiclegrunge512", ms_aDirtTextures_3);

	// Textures which belnd between two images
	InitialiseBlendTextureSingle("tyrewall_dirt", "tyrewall_dirt_dt", ms_aDirtTextures_4);
}
