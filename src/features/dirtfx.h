#pragma once
#include "plugin.h"
#include "core/base.h"
#include "CTxdStore.h"
#include "CClothesBuilder.h"
#include <unordered_map>
#include <array>

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

	struct DirtStages
	{
		std::array<RwTexture *, 16> textures{};
		bool initialized = false;
	};
	static inline std::unordered_map<RwTexture *, DirtStages> m_DirtTextures;
	static inline bool m_bCustomReady = false;

	void Shutdown() override;
	static void ShutdownHook();
	static void InitialiseDirtTextures();
	static void InitialiseBlendTextureSingle(const char *CleanName, const char *DirtName, RwTexture **TextureArray);
	static void InitialiseBlendTextureSingleEx(RwTexture *src, RwTexture *dest, bool overlay);
	static void RegisterVehicleTextures(int model);
	static void InitialiseDirtTextureSingle(const char *name, RwTexture **Array);

protected:
    void Init() override;
    void ReloadConfig() override;
    void Reload(CVehicle *pVeh) override;

public:
    DirtFx() : CBaseFeature("DirtFX", "FEATURES", eFeatureMatrix::DirtFX) {}
	static void ProcessTextures(CVehicle *pVeh, RpMaterial *pMat, RwTexture *baseTexture = nullptr);
};
