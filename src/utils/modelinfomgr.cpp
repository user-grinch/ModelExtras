#include "pch.h"
#include "utils/modelinfomgr.h"
#include <CTxdStore.h>
#include <CCamera.h>
#include <RenderWare.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include "utils/texmgr.h"
#include <NodeName.h>
#include <string_view>
#include "features/carcols.h"
#include "utils/meevents.h"
#include "features/dirtfx.h"
#include "features/plate.h"

using namespace plugin;

extern int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
extern int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat);

static CVehicle *pCurVeh = nullptr;
RwSurfaceProperties &gLightSurfProps = *(RwSurfaceProperties *)0x8A645C;
RwSurfaceProperties gLightSurfPropsOff = {0.45f, 0.0f, 0.0f};

// The upgrade code pulls frames out of CVehicle::m_apModelNodes and hands them to RenderWare
// unchecked. A vehicle whose model is missing the dummy for a slot leaves a null in there and
// the game faults reading RwFrame::objectList (+0x90) or RwFrame::child (+0x98). Everything
// here is reached through ADD_VEHICLE_UPGRADE (opcode 0x6E7), so a script tuning one of those
// vehicles takes the game down. Guarding one call at a time only moved the crash between
// CreateUpgradeAtomic, GetReplacementUpgrade and AddReplacementUpgrade, so cover every such
// call in these functions instead. Skipping costs a missing upgrade part; RenderWare would
// have dereferenced the same null.
//
// This does not cover calls another plugin has already taken over, and VehFuncs owns at least
// one of them, so with VehFuncs loaded the crash can still happen inside vehfuncs.asi. Fixing
// that from here would mean calling back into a function whose signature we can't see, which
// is worse than the crash. See the note in ModelInfoMgr::Init about the real root cause.
static const uint32_t RwFrameForAllObjectsAddr = 0x7F1200;
static const uint32_t RwFrameAddChildAddr = 0x7F0B00;
static const uint32_t GetCurrentAtomicObjectCBAddr = 0x6D33B0;

static void LogSkippedUpgradePart()
{
	static int count = 0;
	if (count < 10)
	{
		++count;
		LOG_VERBOSE("Skipped an upgrade part, the vehicle model has no frame for that slot");
		if (count == 10)
		{
			LOG_VERBOSE("Silencing further upgrade slot messages");
		}
	}
}

static RwFrame *UpgradeFrameForAllObjects(RwFrame *frame, RwObjectCallBack callback, void *data)
{
	if (!frame)
	{
		// GetCurrentAtomicObjectCB fills in a pointer the caller never initialises, so clear
		// it or the caller reads whatever the stack held. Only when that really is the
		// callback being passed, everyone else owns their own data.
		if (data && callback == reinterpret_cast<RwObjectCallBack>(GetCurrentAtomicObjectCBAddr))
		{
			*reinterpret_cast<void **>(data) = nullptr;
		}
		LogSkippedUpgradePart();
		return frame;
	}
	return RwFrameForAllObjects(frame, callback, data);
}

static RwFrame *UpgradeFrameAddChild(RwFrame *parent, RwFrame *child)
{
	if (!parent)
	{
		LogSkippedUpgradePart();
		return parent;
	}
	return RwFrameAddChild(parent, child);
}

// Only take over calls that still point at the vanilla RenderWare functions, matched on the
// resolved target so this can't trip over data that merely starts with an E8 byte. Calls that
// another plugin has already redirected are left alone: hooking those means calling back into
// a function whose signature and calling convention we don't know, which corrupts the stack.
static size_t GuardUpgradeFrameCalls(uint32_t start, uint32_t end)
{
	size_t patched = 0;
	for (uint32_t addr = start; addr < end; ++addr)
	{
		if (*reinterpret_cast<uint8_t *>(addr) != 0xE8)
		{
			continue;
		}

		uint32_t target = addr + 5 + *reinterpret_cast<int32_t *>(addr + 1);
		if (target == RwFrameForAllObjectsAddr)
		{
			patch::ReplaceFunctionCall(addr, (void *)UpgradeFrameForAllObjects);
			++patched;
		}
		else if (target == RwFrameAddChildAddr)
		{
			patch::ReplaceFunctionCall(addr, (void *)UpgradeFrameAddChild);
			++patched;
		}
	}
	return patched;
}

void ModelInfoMgr::Init()
{
	// Nop frame collasping
	//
	// Worth a look if upgrade parts go missing: this is the code that fills in
	// CVehicle::m_apModelNodes, and the guards below fire constantly on some servers,
	// meaning those slots come out null. Either those models genuinely have no dummy for
	// the slot, or keeping the hierarchy leaves the nodes unpopulated and this is where
	// the real fix belongs.
	patch::Nop(0x4C8E53, 5);
	patch::Nop(0x4C8F6E, 5);

	// CreateUpgradeAtomic, AddReplacementUpgrade and GetReplacementUpgrade sit together, and
	// AddUpgrade is the other caller that reaches for a model node.
	size_t guarded = GuardUpgradeFrameCalls(0x6D3300, 0x6D3C00);
	guarded += GuardUpgradeFrameCalls(0x6DF900, 0x6DFC00);
	if (guarded > 0)
	{
		LOG_VERBOSE("Guarded {} vehicle upgrade frame calls", guarded);
	}
	else
	{
		LOG(ERROR) << "Found no vehicle upgrade frame calls to guard, the addresses may have moved";
	}

	patch::ReplaceFunctionCall(0x5532A9, (void *)ModelInfoMgr::SetupRender);
	patch::ReplaceFunction(0x4C8220, (void *)ModelInfoMgr::SetEditableMaterialsCB);

	Events::initScriptsEvent += []()
	{
		gLightSurfProps.ambient = gConfig.ReadFloat("VISUAL", "MaterialAmbientOn", 50.0f);
		gLightSurfProps.diffuse = gConfig.ReadFloat("VISUAL", "MaterialDiffuseOn", 0.0f);
		gLightSurfProps.specular = 0.0f;
		gLightSurfPropsOff.ambient = gConfig.ReadFloat("VISUAL", "MaterialAmbientOff", gLightSurfPropsOff.ambient);
		gLightSurfPropsOff.diffuse = 0.0f;
		gLightSurfPropsOff.specular = 0.0f;
	};

	MEEvents::vehRenderEvent.before += [](CVehicle *pVeh)
	{
		if (!pVeh || !pVeh->m_pRwClump)
		{
			return;
		}

		// Wait for VehFuncs to init extras
		auto &data = m_VehData.Get(pVeh);
		if (data.nFrameCount > 10)
		{
			ModelInfoMgr::OnRender(pVeh);
		}
		else if (data.nFrameCount == 10)
		{
			ModelInfoMgr::FindDummies(pVeh, (RwFrame *)pVeh->m_pRwClump->object.parent);
			data.nFrameCount++;
		}
		else
		{
			data.nFrameCount++;
		}
	};

	MEEvents::heliRenderEvent.after += [](CVehicle *pVeh)
	{
		if (CModelInfo::IsHeliModel(pVeh->m_nModelIndex))
		{
			ModelInfoMgr::OnRender(pVeh);
		}
	};
	// Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model)
	// {
	//     ModelInfoMgr::FindDummies(pVeh, (RwFrame *)pVeh->m_pRwClump->object.parent);
	// };
}

void ModelInfoMgr::RegisterRender(const RenderCallback_t &render)
{
	renders.push_back(render);
};

void ModelInfoMgr::RegisterDummy(const DummyCallback_t &function)
{
	dummy.push_back(function);
};

void ModelInfoMgr::EnableMaterial(CVehicle *pVeh, eMaterialType type)
{
	auto &data = m_VehData.Get(pVeh);
	data.m_MatStatus[type] = true;
}

void ModelInfoMgr::EnableSirenMaterial(CVehicle *pVeh, int idx)
{
	auto &data = m_VehData.Get(pVeh);
	data.m_SirenStatus[idx] = true;
}

void ModelInfoMgr::EnableStrobeMaterial(CVehicle *pVeh, int idx)
{
	auto &data = m_VehData.Get(pVeh);
	data.m_StrobeStatus[idx] = true;
}

void ModelInfoMgr::FindDummies(CVehicle *vehicle, RwFrame *frame)
{
	if (frame)
	{
		if (RwFrame *nextFrame = frame->child)
		{
			FindDummies(vehicle, nextFrame);
		}

		if (RwFrame *nextFrame = frame->next)
		{
			FindDummies(vehicle, nextFrame);
		}

		std::string_view nodeName = GetFrameNodeName(frame);
		for (const auto &e : dummy)
		{
			e(vehicle, frame, nodeName);
		}
	}
};

void ModelInfoMgr::Reload(CVehicle *pVeh)
{
	if (pVeh->m_pRwClump)
	{
		RwFrame *frame = reinterpret_cast<RwFrame *>(pVeh->m_pRwClump->object.parent);
		FindDummies(pVeh, frame);
	}
}

void ModelInfoMgr::OnRender(CVehicle *vehicle)
{
	if (!renders.empty())
	{
		for (const auto &e : renders)
		{
			e(vehicle);
		}
	}
}

void ModelInfoMgr::RegisterMaterial(const MaterialCallback_t &mat)
{
	materials.push_back(mat);
}

void ModelInfoMgr::RegisterMaterialColProvider(const MaterialColProviderCallback_t &mat)
{
	matColProviders.push_back(mat);
}

void ModelInfoMgr::SetupRender(CVehicle *ptr)
{
	pCurVeh = ptr;
	auto &data = m_VehData.Get(pCurVeh);
	ptr->SetupRender();
	for (int i = 0; i < eMaterialType::TotalMaterial; i++)
	{
		data.m_MatStatus[i] = false;
	}

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		data.m_SirenStatus[i] = false;
	}

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		data.m_StrobeStatus[i] = false;
	}
}

struct tRestoreEntry
{
	void *m_pAddress;
	void *m_pValue;
};

// SetEditableMaterials collects what has to be put back after the vehicle is drawn in a
// fixed array and marks the end with a null address, so there is no count to compare
// against. It holds 256 entries, ending where CVehicleModelInfo::ms_lightsOn and the
// texture pointers beside it begin. The vanilla callback spends at most one entry per
// material, this one spends two whenever a light is lit, so a model carrying a few
// hundred materials writes past the end and into those texture pointers - which is what
// the renderer faults on. Stop recording once it's full; the material then keeps its own
// colour and texture, which merely looks wrong.
static tRestoreEntry *const gRestoreEntries = reinterpret_cast<tRestoreEntry *>(0xB4DBE8);
static constexpr ptrdiff_t RESTORE_ENTRY_COUNT = 256;
// A pointer a little past the end is this array overrun by someone else, not a different
// one, so keep enforcing there. Anything far away belongs to another plugin, whose size
// we can't know: leave those alone rather than refuse every material.
static constexpr ptrdiff_t RESTORE_ENTRY_RANGE = RESTORE_ENTRY_COUNT * 2;

static bool CanStoreRestoreEntries(tRestoreEntry **ppEntries, ptrdiff_t needed)
{
	tRestoreEntry *pEntry = *ppEntries;
	if (pEntry < gRestoreEntries || pEntry >= gRestoreEntries + RESTORE_ENTRY_RANGE)
	{
		return true;
	}

	// One entry has to stay free for the terminator SetEditableMaterials writes.
	return (pEntry - gRestoreEntries) + needed + 1 <= RESTORE_ENTRY_COUNT;
}

static void LogRestoreEntriesFull()
{
	static int count = 0;
	if (count < 10)
	{
		++count;
		LOG_VERBOSE("Ran out of material restore entries, this model has too many editable materials");
		if (count == 10)
		{
			LOG_VERBOSE("Silencing further material restore entry messages");
		}
	}
}

MatStateColor ModelInfoMgr::FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat, eMaterialType type)
{
	MatStateColor col = {DEFAULT_MAT_COL, DEFAULT_MAT_COL};
	for (auto &e : matColProviders)
	{
		col = e(pVeh, pMat, type);
		if (col.on != DEFAULT_MAT_COL || col.off != DEFAULT_MAT_COL)
		{
			break;
		}
	}
	return col;
}

eMaterialType ModelInfoMgr::FetchMaterialType(CVehicle *pVeh, RpMaterial *pMat)
{
	eMaterialType matType = eMaterialType::UnknownMaterial;

	for (auto &e : materials)
	{
		eMaterialType type = e(pVeh, pMat);
		if (type != eMaterialType::UnknownMaterial)
		{
			matType = type;
			break;
		}
	}
	return matType;
}

RpMaterial *ModelInfoMgr::SetEditableMaterialsCB(RpMaterial *material, void *data)
{
	if (!material)
	{
		return material;
	}

	tRestoreEntry **ppEntries = reinterpret_cast<tRestoreEntry **>(data);
	if (material->texture)
	{
		bool isRemapTex = RwTextureGetName(RpMaterialGetTexture(material))[0] == '#';
		if (isRemapTex)
		{
			if (CVehicleModelInfo::ms_pRemapTexture && CanStoreRestoreEntries(ppEntries, 1))
			{
				(*ppEntries)->m_pAddress = &material->texture;
				(*ppEntries)->m_pValue = material->texture;
				(*ppEntries)++;
				material->texture = CVehicleModelInfo::ms_pRemapTexture;
			}
		}
		else if (pCurVeh)
		{
			DirtFx::ProcessTextures(pCurVeh, material);
			LicensePlate::ProcessTextures(pCurVeh, material);
		}
	}

	// Only SetupRender assigns pCurVeh, and this callback also runs when a model is
	// being set up rather than a vehicle drawn. SilentPatch walks an atomic's materials
	// through the vanilla function for its special vehicle check, and the vanilla one
	// never needed a vehicle. Everything past the remap swap above does, so leave the
	// materials alone until there's a vehicle to read them for.
	if (!pCurVeh)
	{
		return material;
	}

	eMaterialType iLightIndex = FetchMaterialType(pCurVeh, material);

	if (iLightIndex != eMaterialType::UnknownMaterial)
	{
		auto &data = m_VehData.Get(pCurVeh);

		bool lightOn = false;
		data.m_MatAvail[iLightIndex] = true;

		if (iLightIndex == eMaterialType::SirenLight)
		{
			int idx = GetSirenIndex(pCurVeh, material);
			if (idx >= 0 && idx < MAX_LIGHTS)
			{
				lightOn = data.m_SirenStatus[idx];
			}
		}
		else if (iLightIndex == eMaterialType::StrobeLight)
		{
			int idx = GetStrobeIndex(pCurVeh, material);
			if (idx >= 0 && idx < MAX_LIGHTS)
			{
				lightOn = data.m_StrobeStatus[idx];
			}
		}
		else if (iLightIndex != eMaterialType::UnknownMaterial)
		{
			lightOn = data.m_MatStatus[iLightIndex];
		}

		MatStateColor matCol = FetchMaterialCol(pCurVeh, material, iLightIndex);
		if (!CanStoreRestoreEntries(ppEntries, lightOn ? 2 : 1))
		{
			LogRestoreEntriesFull();
			return material;
		}

		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = matCol.on.r;
		RpMaterialGetColor(material)->green = matCol.on.g;
		RpMaterialGetColor(material)->blue = matCol.on.b;

		if (lightOn)
		{
			(*ppEntries)->m_pAddress = &material->texture;
			(*ppEntries)->m_pValue = material->texture;
			(*ppEntries)++;

			if (material->texture)
			{
				if (material->texture == CVehicleModelInfo::ms_pLightsTexture)
				{
					material->texture = CVehicleModelInfo::ms_pLightsOnTexture;
				}
				else
				{
					RwTexture *pTex = TextureMgr::FindOnTextureInDict(material, material->texture->dict);
					if (pTex)
					{
						material->texture = pTex;
					}
					else
					{
						LOG_VERBOSE("Expected an 'on' texture for {} but none found", material->texture->name);
					}
				}
			}
			material->surfaceProps = gLightSurfProps;
		}
		else
		{
			RpMaterialGetColor(material)->red = matCol.off.r;
			RpMaterialGetColor(material)->green = matCol.off.g;
			RpMaterialGetColor(material)->blue = matCol.off.b;
			material->surfaceProps = gLightSurfPropsOff;
		}
	}
	else
	{
		CRGBA col = {255, 255, 255, 255};
		if (Carcols::GetColor(pCurVeh, material, col) && CanStoreRestoreEntries(ppEntries, 1))
		{
			(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
			(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
			(*ppEntries)++;

			RpMaterialGetColor(material)->red = col.r;
			RpMaterialGetColor(material)->green = col.g;
			RpMaterialGetColor(material)->blue = col.b;
		}
	}

	return material;
}

bool ModelInfoMgr::IsMaterialAvailable(CVehicle *pVeh, eMaterialType type)
{
	auto &data = m_VehData.Get(pVeh);
	return data.m_MatAvail[type];
}