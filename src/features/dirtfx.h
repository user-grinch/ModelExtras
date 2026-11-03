#pragma once
#include "plugin.h"
#include "core/base.h"
#include <unordered_map>
#include <d3d9.h>

using namespace plugin;

class DirtFx : public CBaseFeature
{
private:
	static inline bool m_bEnabled = false;
	static inline RwTexture **ms_aDirtTextures = reinterpret_cast<RwTexture **>(0xC02BD0);

	struct DirtTarget
	{
		RwTexture *pCleanTexture = nullptr;
		RwTexture *pDirtTexture = nullptr;
		RwTexture *pTargetTexture = nullptr;
		int lastDirtLevel = -1;
		bool failed = false;
	};

	static inline std::unordered_map<RwTexture *, DirtTarget> m_DirtTextures;
	static inline IDirect3DPixelShader9 *m_pPixelShader = nullptr;

	void Shutdown() override;

	static bool EnsureGpuResources();
	static RwTexture *FindDirtTexture(RwTexture *clean);
	static RwTexture *CreateTargetTexture(RwTexture *clean, RwTexture *dirt);
	static bool RenderDirtBlendTexture(RwTexture *pTargetTex, RwTexture *pCleanTex, RwTexture *pDirtTex, float dirtFactor);

protected:
	void Init() override;
	void ReloadConfig() override;

public:
	DirtFx() : CBaseFeature("DirtFX", "FEATURES", eFeatureMatrix::DirtFX) {}
	static void ProcessTextures(CVehicle *pVeh, RpMaterial *pMat, RwTexture *baseTexture = nullptr);
};
