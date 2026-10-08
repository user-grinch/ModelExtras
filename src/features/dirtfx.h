#pragma once
#include "plugin.h"
#include "core/base.h"
#include "CTxdStore.h"
#include "CClothesBuilder.h"
#include <map>
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
	// Key by the actual dirt/clean textures, not names shared by unrelated TXDs.
	struct DirtStages
	{
		std::array<RwTexture *, 16> textures{};
		RwUInt16 requested = 0;
		RwUInt16 attempted = 0;
		bool overlay = false;
		std::array<size_t, 16> bytes{};
		std::array<uint64_t, 16> lastUsed{};
	};
	static inline std::map<RwTexture *, std::map<RwTexture *, DirtStages>> m_DirtTextures;
	static inline bool m_bCustomReady = false;
	static constexpr size_t CustomCacheLimit = 128u * 1024u * 1024u;
	static inline size_t m_nCustomCacheBytes = 0;
	static inline uint64_t m_nTextureUse = 0;

	void Shutdown() override;
	static void ShutdownHook();
	static void InitialiseDirtTextures();
	static void InitialiseBlendTextureSingle(const char *CleanName, const char *DirtName, RwTexture **TextureArray);
	static void InitialiseBlendTextureSingleEx(RwTexture *src, RwTexture *dest, bool overlay);
	static void InitialiseDirtStage(RwTexture *src, RwTexture *dest, DirtStages &stages, int level);
	static void ProcessDirtJob();
	static void ReleaseDirtStage(DirtStages &stages, int level);
	static void DestroyDirtStages(DirtStages &stages);
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
