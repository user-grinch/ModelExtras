#include "pch.h"
#include "dirtfx.h"
#include "defines.h"
#include "utils/modelinfomgr.h"
#include <d3d9.h>
#include <cmath>
#include <bit>
#include <algorithm>
#include <format>
#include <string_view>

using namespace plugin;

// 11-line HLSL ps_2_0 pixel shader compiled with fxc (Windows Kits 10 SDK).
// Blends clean body texture (s0) with dirt overlay (s1) scaled by dirt level (c0.x).
static constexpr unsigned char g_DirtPixelShaderBytecode[] = {
      0,   2, 255, 255, 254, 255,  53,   0,  67,  84,  65,  66,  28,   0,   0,   0,
    167,   0,   0,   0,   0,   2, 255, 255,   3,   0,   0,   0,  28,   0,   0,   0,
      0,   1,   0,   0, 160,   0,   0,   0,  88,   0,   0,   0,   3,   0,   0,   0,
      1,   0,   2,   0,  96,   0,   0,   0,   0,   0,   0,   0, 112,   0,   0,   0,
      3,   0,   1,   0,   1,   0,   6,   0, 120,   0,   0,   0,   0,   0,   0,   0,
    136,   0,   0,   0,   2,   0,   0,   0,   1,   0,   2,   0, 144,   0,   0,   0,
      0,   0,   0,   0,  98, 111, 100, 121,   0, 171, 171, 171,   4,   0,  12,   0,
      1,   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0, 100, 105, 114, 116,
      0, 171, 171, 171,   4,   0,  12,   0,   1,   0,   1,   0,   1,   0,   0,   0,
      0,   0,   0,   0, 108, 101, 118, 101, 108,   0, 171, 171,   0,   0,   3,   0,
      1,   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0, 112, 115,  95,  50,
     95,  48,   0,  77, 105,  99, 114, 111, 115, 111, 102, 116,  32,  40,  82,  41,
     32,  72,  76,  83,  76,  32,  83, 104,  97, 100, 101, 114,  32,  67, 111, 109,
    112, 105, 108, 101, 114,  32,  49,  48,  46,  49,   0, 171,  31,   0,   0,   2,
      0,   0,   0, 128,   0,   0,   3, 176,  31,   0,   0,   2,   0,   0,   0, 144,
      0,   8,  15, 160,  31,   0,   0,   2,   0,   0,   0, 144,   1,   8,  15, 160,
     66,   0,   0,   3,   0,   0,  15, 128,   0,   0, 228, 176,   0,   8, 228, 160,
     66,   0,   0,   3,   1,   0,  15, 128,   0,   0, 228, 176,   1,   8, 228, 160,
      5,   0,   0,   3,   2,   0,   8, 128,   1,   0, 255, 128,   0,   0,   0, 160,
     18,   0,   0,   4,   3,   0,   7, 128,   2,   0, 255, 128,   1,   0, 228, 128,
      0,   0, 228, 128,   4,   0,   0,   4,   3,   0,  24, 128,   1,   0, 255, 128,
      0,   0,   0, 160,   0,   0, 255, 128,   1,   0,   0,   2,   0,   8,  15, 128,
      3,   0, 228, 128, 255, 255,   0,   0
};

static inline IDirect3DBaseTexture9 *GetD3DTexture(RwRaster *raster)
{
	if (!raster) return nullptr;
	RwRaster *parent = raster->parent ? raster->parent : raster;
	const uint32_t offset = *reinterpret_cast<const uint32_t *>(0xB4E9E0);
	return *reinterpret_cast<IDirect3DBaseTexture9 **>(reinterpret_cast<uint8_t *>(parent) + offset);
}

struct DirtVertex
{
	float x, y, z, rhw;
	float u, v;
};

bool DirtFx::EnsureGpuResources()
{
	if (m_pPixelShader) return true;

	auto *pDevice = *reinterpret_cast<IDirect3DDevice9 **>(0xC97C28);
	if (!pDevice) return false;

	HRESULT hr = pDevice->CreatePixelShader(
		reinterpret_cast<const DWORD *>(g_DirtPixelShaderBytecode),
		&m_pPixelShader);
	return SUCCEEDED(hr) && m_pPixelShader != nullptr;
}

RwTexture *DirtFx::FindDirtTexture(RwTexture *clean)
{
	if (!clean || !clean->dict || !clean->name) return nullptr;
	const std::string_view name(clean->name);
	if (name.empty()) return nullptr;

	auto tryFind = [dict = clean->dict](std::string_view base, std::string_view suffix) -> RwTexture *
	{
		char buf[64];
		if (base.size() + suffix.size() >= sizeof(buf)) return nullptr;
		auto res = std::format_to_n(buf, sizeof(buf) - 1, "{}{}", base, suffix);
		*res.out = '\0';

		if (RwTexture *dirt = RwTexDictionaryFindNamedTexture(dict, buf))
			return dirt;

		// SA paintjob textures in dictionaries start with '#', e.g. #emap... -> remap...
		if (buf[0] == '#')
		{
			buf[0] = 'r';
			return RwTexDictionaryFindNamedTexture(dict, buf);
		}
		return nullptr;
	};

	// 1. Direct search for companion dirt suffixes (_dt for BTLC/tyres, _dirt/_d for IVF alpha overlays)
	for (std::string_view suffix : {"_dt", "_dirt", "_d"})
	{
		if (RwTexture *dirt = tryFind(name, suffix))
			return dirt;
	}

	// 2. A random remap may share its base's IVF overlay. A complete _dt image
	// cannot be shared: that would replace the selected livery at full dirt.
	const auto remapPos = [name]() {
		for (size_t i = 0; i + 6 <= name.size(); ++i)
		{
			if (_strnicmp(name.data() + i, "_remap", 6) == 0) return i;
		}
		return std::string_view::npos;
	}();

	if (remapPos > 0 && remapPos != std::string_view::npos && name[0] != '#')
	{
		std::string_view base = name.substr(0, remapPos);
		for (std::string_view suffix : {"_dirt", "_d"})
		{
			if (RwTexture *dirt = tryFind(base, suffix))
				return dirt;
		}
	}

	return nullptr;
}

RwTexture *DirtFx::CreateTargetTexture(RwTexture *clean, RwTexture *dirt)
{
	if (!clean || !clean->raster || !dirt || !dirt->raster) return nullptr;

	const uint32_t rawW = static_cast<uint32_t>(std::max({1, clean->raster->width, dirt->raster->width}));
	const uint32_t rawH = static_cast<uint32_t>(std::max({1, clean->raster->height, dirt->raster->height}));
	const uint32_t width  = std::clamp(std::bit_ceil(rawW), 256u, 2048u);
	const uint32_t height = std::clamp(std::bit_ceil(rawH), 256u, 2048u);

	RwRaster *pRaster = RwRasterCreate(width, height, 0, rwRASTERTYPECAMERATEXTURE);
	if (!pRaster) return nullptr;

	RwTexture *pTexture = RwTextureCreate(pRaster);
	if (!pTexture)
	{
		RwRasterDestroy(pRaster);
		return nullptr;
	}

	RwTextureSetName(pTexture, clean->name);
	pTexture->filterAddressing = (clean->filterAddressing & 0xFF00) | rwFILTERLINEAR;
	return pTexture;
}

bool DirtFx::RenderDirtBlendTexture(RwTexture *pTargetTex, RwTexture *pCleanTex, RwTexture *pDirtTex, float dirtFactor)
{
	if (!pTargetTex || !pTargetTex->raster || !pCleanTex || !pCleanTex->raster || !pDirtTex || !pDirtTex->raster) return false;
	if (!EnsureGpuResources()) return false;

	auto *pDevice = *reinterpret_cast<IDirect3DDevice9 **>(0xC97C28);
	if (!pDevice || pDevice->TestCooperativeLevel() != D3D_OK) return false;

	auto *pCleanD3D = GetD3DTexture(pCleanTex->raster);
	auto *pDirtD3D  = GetD3DTexture(pDirtTex->raster);
	RwRaster *targetRaster = pTargetTex->raster;
	auto *pD3DTarget = static_cast<IDirect3DTexture9 *>(GetD3DTexture(targetRaster));
	if (!pCleanD3D || !pDirtD3D || !pD3DTarget) return false;

	IDirect3DSurface9 *pTargetSurface = nullptr;
	if (FAILED(pD3DTarget->GetSurfaceLevel(0, &pTargetSurface)) || !pTargetSurface) return false;

	IDirect3DStateBlock9 *pStateBlock = nullptr;
	if (FAILED(pDevice->CreateStateBlock(D3DSBT_ALL, &pStateBlock)) || !pStateBlock)
	{
		pTargetSurface->Release();
		return false;
	}
	pStateBlock->Capture();

	// Preserve active render targets (including Proper Shaders MRT G-buffers), depth-stencil, and viewport.
	// Restoring them explicitly prevents any clobbering of shader mod deferred pipelines.
	IDirect3DSurface9 *pSavedRT[4] = { nullptr, nullptr, nullptr, nullptr };
	IDirect3DSurface9 *pSavedDS = nullptr;
	D3DVIEWPORT9 savedViewport{};

	for (DWORD i = 0; i < 4; ++i)
	{
		pDevice->GetRenderTarget(i, &pSavedRT[i]);
	}
	pDevice->GetDepthStencilSurface(&pSavedDS);
	pDevice->GetViewport(&savedViewport);

	pDevice->SetRenderTarget(0, pTargetSurface);
	pTargetSurface->Release();
	for (DWORD i = 1; i < 4; ++i)
	{
		if (pSavedRT[i])
		{
			pDevice->SetRenderTarget(i, nullptr);
		}
	}
	pDevice->SetDepthStencilSurface(nullptr);

	const float w = static_cast<float>(targetRaster->width);
	const float h = static_cast<float>(targetRaster->height);
	const D3DVIEWPORT9 vp{
		.X = 0,
		.Y = 0,
		.Width = static_cast<DWORD>(targetRaster->width),
		.Height = static_cast<DWORD>(targetRaster->height),
		.MinZ = 0.0f,
		.MaxZ = 1.0f
	};
	pDevice->SetViewport(&vp);

	// Direct3D 9 half-pixel correction (-0.5f) for pixel-perfect 1:1 screen quad mapping
	const DirtVertex verts[4] = {
		{ -0.5f,     -0.5f,     0.0f, 1.0f, 0.0f, 0.0f },
		{ w - 0.5f,  -0.5f,     0.0f, 1.0f, 1.0f, 0.0f },
		{ -0.5f,     h - 0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
		{ w - 0.5f,  h - 0.5f,  0.0f, 1.0f, 1.0f, 1.0f },
	};

	pDevice->SetTexture(0, pCleanD3D);
	pDevice->SetTexture(1, pDirtD3D);

	float consts[4] = { dirtFactor, 0.0f, 0.0f, 0.0f };
	pDevice->SetPixelShaderConstantF(0, consts, 1);

	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
	pDevice->SetVertexShader(nullptr);
	pDevice->SetPixelShader(m_pPixelShader);

	pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	pDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);

	pDevice->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	pDevice->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, 0);

	// Explicitly force all critical render states. In deferred pipelines (Deferred SkyGFX,
	// Proper Shaders) or shadow passes, COLORWRITEENABLE may be zero or alpha-only, and
	// alpha-test / sRGB / scissor may be active, which would prevent color writes to pTargetSurface.
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
	pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
	pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	HRESULT hr = pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(DirtVertex));

	// Restore original render targets, depth-stencil, viewport, and D3D9 pipeline states
	for (DWORD i = 0; i < 4; ++i)
	{
		pDevice->SetRenderTarget(i, pSavedRT[i]);
		if (pSavedRT[i]) pSavedRT[i]->Release();
	}
	pDevice->SetDepthStencilSurface(pSavedDS);
	if (pSavedDS) pSavedDS->Release();
	pDevice->SetViewport(&savedViewport);

	pStateBlock->Apply();
	pStateBlock->Release();

	return SUCCEEDED(hr);
}

void DirtFx::ProcessTextures(CVehicle *pVeh, RpMaterial *pMat, RwTexture *baseTexture)
{
	if (!m_bEnabled || !pVeh || !pMat || !pMat->texture) return;

	const char *rawName = pMat->texture->name;
	if (!rawName) return;

	const float dirtLevel = std::isfinite(pVeh->m_fDirtLevel) ? pVeh->m_fDirtLevel : 0.0f;
	const int dirtLvl = static_cast<int>(std::clamp(dirtLevel, 0.0f, 15.0f));

	// 1. Vanilla SA generic vehiclegrunge256 (pre-baked by game engine at 0xC02BD0)
	if (_stricmp(rawName, "vehiclegrunge256") == 0)
	{
		if (ms_aDirtTextures && ms_aDirtTextures[dirtLvl])
		{
			ModelInfoMgr::RegisterRestore(&pMat->texture, pMat->texture);
			pMat->texture = ms_aDirtTextures[dirtLvl];
		}
		return;
	}

	// 2. Custom vehicle textures (IVF _dirt/_d, BTLC _dt, tyre sidewalls, paintjobs)
	if (dirtLvl > 0)
	{
		auto it = m_DirtTextures.find(pMat->texture);
		if (it == m_DirtTextures.end())
		{
			RwTexture *dirt = FindDirtTexture(pMat->texture);
			if (!dirt && baseTexture) dirt = FindDirtTexture(baseTexture);

			RwTexture *targetTex = dirt ? CreateTargetTexture(pMat->texture, dirt) : nullptr;
			it = m_DirtTextures.emplace(pMat->texture, DirtTarget{
				.pCleanTexture = pMat->texture,
				.pDirtTexture = dirt,
				.pTargetTexture = targetTex,
				.failed = (targetTex == nullptr)
			}).first;
		}

		if (!it->second.failed)
		{
			if (it->second.lastDirtLevel != dirtLvl)
			{
				const float dirtFactor = static_cast<float>(dirtLvl) / 15.0f;
				if (RenderDirtBlendTexture(it->second.pTargetTexture, it->second.pCleanTexture, it->second.pDirtTexture, dirtFactor))
				{
					it->second.lastDirtLevel = dirtLvl;
				}
				else
				{
					// Blend failed (e.g. device lost or state rejection); keep clean texture and retry next frame
					return;
				}
			}
			ModelInfoMgr::RegisterRestore(&pMat->texture, pMat->texture);
			pMat->texture = it->second.pTargetTexture;
		}
	}
}

void DirtFx::Init()
{
	ReloadConfig();
	Events::attachRwPluginsEvent += []()
	{
		// Release cached copies when either source streams out; pointer reuse must
		// never give another model's textures an old dirt cache.
		RwTextureRegisterPlugin(0, PLUGIN_ID_NUM, nullptr,
			[](void *object, RwInt32, RwInt32) -> void *
			{
				static bool s_bInDtor = false;
				if (s_bInDtor) return object;
				s_bInDtor = true;

				auto *texture = static_cast<RwTexture *>(object);
				for (auto it = m_DirtTextures.begin(); it != m_DirtTextures.end(); )
				{
					if (it->first == texture || it->second.pDirtTexture == texture || it->second.pTargetTexture == texture)
					{
						if (it->second.pTargetTexture && it->second.pTargetTexture != texture)
						{
							RwTexture *pTarget = it->second.pTargetTexture;
							it->second.pTargetTexture = nullptr;
							RwTextureDestroy(pTarget);
						}
						it = m_DirtTextures.erase(it);
					}
					else
					{
						++it;
					}
				}
				s_bInDtor = false;
				return object;
			}, nullptr);
	};

	// On Alt-Tab / D3D9 device reset, mark all cached targets as unblended (-1)
	// so the next frame immediately re-renders the clean/dirt blend into the restored surface.
	Events::d3dResetEvent.after += []()
	{
		for (auto &[tex, target] : m_DirtTextures)
		{
			target.lastDirtLevel = -1;
		}
	};
}

void DirtFx::ReloadConfig()
{
	CBaseFeature::ReloadConfig();
	m_bEnabled = m_bActive;
}

void DirtFx::Shutdown()
{
	for (auto &[tex, target] : m_DirtTextures)
	{
		if (target.pTargetTexture)
		{
			RwTexture *pTarget = target.pTargetTexture;
			target.pTargetTexture = nullptr;
			RwTextureDestroy(pTarget);
		}
	}
	m_DirtTextures.clear();

	if (m_pPixelShader)
	{
		m_pPixelShader->Release();
		m_pPixelShader = nullptr;
	}
}
