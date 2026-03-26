#pragma once
#include "plugin.h"
#include "core/base.h"
#include "CTxdStore.h"
#include "CClothesBuilder.h"
#include <vector>
#include <map>

using namespace plugin;

// Thanks to GTA BTLC (D4DJ)
class DirtFx : public CBaseFeature
{
private:
	// Dirttextures
	static inline bool m_bEnabled = false;
	static inline RwTexture **ms_aDirtTextures = (RwTexture **)0xC02BD0;
	static inline RwTexture *ms_aDirtTextures_2[16] = {};
	static inline RwTexture *ms_aDirtTextures_3[16] = {};
	static inline RwTexture *ms_aDirtTextures_4[16] = {};
	static inline RwTexture *ms_aDirtTextures_5[16] = {};
	static inline RwTexture *ms_aDirtTextures_6[16] = {};
	static inline std::map<std::string, std::vector<RwTexture *>> m_DirtTextures;

	void Shutdown() override;
	static void ShutdownHook();
	static void InitialiseDirtTexture();
	static void InitialiseDirtTextures();
	static void InitialiseBlendTextureSingle(const char *CleanName, const char *DirtName, RwTexture **TextureArray);
	static void InitialiseBlendTextureSingleEx(RwTexture *src, RwTexture *dest);
	static void InitialiseDirtTextureSingle(const char *name, RwTexture **Array);

protected:
    void Init() override;

public:
	public:
    DirtFx() : CBaseFeature("DirtFX", "FEATURES", eFeatureMatrix::DirtFX) {}
	static void ProcessTextures(CVehicle *pVeh, RpMaterial *pMat);
};