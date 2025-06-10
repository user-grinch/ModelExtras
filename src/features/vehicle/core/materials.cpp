#include "pch.h"
#include "materials.h"
#include <CTxdStore.h>
#include <RenderWare.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include "texmgr.h"
#include "colors.h"

RwSurfaceProperties &gLightSurfProps = *(RwSurfaceProperties *)0x8A645C;

RwTexture *FindTextureInDict(RpMaterial *pMat, RwTexDictionary *pDict)
{
	const std::string baseName = pMat->texture->name;

	/*
		This needs to be handled separately
		Apperently VehFuncs allows for more vehicleXX.txds
		This needs to be handled someday too.

		Directly addding it to the texNames vector causes graphical annomolies
	*/
	// if (RpMaterialGetTexture(pMat) == CVehicleModelInfo::ms_pLightsTexture)
	// {
	// 	CVehicleModelInfo::ms_pLightsOnTexture->refCount++;
	// 	return CVehicleModelInfo::ms_pLightsOnTexture;
	// }

	// texture glitch fix
	const std::vector<std::string> texNames = {
		// baseName,
		baseName + "on",
		baseName + "_on",
		// Don't touch this
		// "sirenlighton",
		// "sirenlight_on",
		// "vehiclelightson128"
	};

	RwTexture *pTex = nullptr;
	for (const auto &name : texNames)
	{
		pTex = RwTexDictionaryFindNamedTexture(pDict, name.c_str());
		if (pTex)
		{
			break;
		}
	}
	return pTex;
}

MEMaterial::~MEMaterial() {
	// pGameMat->texture = pTextureOff;
	// CVehicleModelInfo::ms_pLightsOnTexture->refCount--;
}

MEMaterial::MEMaterial(RpMaterial *material, eDummyPos pos)
{
	if (!material || !material->texture)
	{
		return;
	}

	pGameMat = material;
	pTextureOff = material->texture;
	pTextureOn = material->texture;
	Pos = pos;
	Color = {material->color.red, material->color.green, material->color.blue, material->color.alpha};

	RwTexture *pTexture = FindTextureInDict(material, material->texture->dict);

	if (pTexture)
	{
		pTextureOn = pTexture;
	}
}

void ModelMgr::Register(std::function<RpMaterial *(CVehicle *, RpMaterial *, CRGBA)> function)
{
	functions.push_back(function);
};

void ModelMgr::RegisterRender(std::function<void(CVehicle *)> render)
{
	renders.push_back(render);
};

void ModelMgr::RegisterDummy(std::function<void(CVehicle *, RwFrame *, std::string, bool)> function)
{
	dummy.push_back(function);
};

void ModelMgr::OnModelSet(CVehicle *vehicle, int model)
{
	currentVehicle = vehicle;

	RpClumpForAllAtomics(vehicle->m_pRwClump, [](RpAtomic *atomic, void *data)
						 {
		if (!atomic->geometry)
			return atomic;

		RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial* material, void* data) {
			if (!material || !material->texture)
				return material;

			/*
			*	Note: Material data need to be model based
			*		  Dummy data should be entity based
			*		  Don't change it
			*/
			if (materials[currentVehicle->m_nModelIndex].contains(material))
				return material;

			if (matColBackup[currentVehicle->m_nModelIndex].contains(material)) {
				material->color = matColBackup[currentVehicle->m_nModelIndex][material];
			}
			else {
				matColBackup[currentVehicle->m_nModelIndex][material] = material->color;
			}
			for (auto& e : functions) {
				e(currentVehicle, material, material->color);
			}

			materials[currentVehicle->m_nModelIndex][material] = true;

			return material;
		}, atomic);

		return atomic; }, nullptr);

	ModelMgr::FindDummies(vehicle, (RwFrame *)vehicle->m_pRwClump->object.parent);
};

void ModelMgr::FindDummies(CVehicle *vehicle, RwFrame *frame, bool parent, bool print)
{
	if (frame)
	{
		const std::string name = GetFrameNodeName(frame);

		if (print)
		{
			gLogger->info(name);
		}

		if (RwFrame *nextFrame = frame->child)
		{
			FindDummies(vehicle, nextFrame, (RwFrameGetParent(frame)) ? (true) : (false), print);
		}

		if (RwFrame *nextFrame = frame->next)
		{
			FindDummies(vehicle, nextFrame, parent, print);
		}

		for (auto e : dummy)
		{
			e(currentVehicle, frame, name, parent);
		}
	}
};

void ModelMgr::StoreMaterial(std::pair<unsigned int *, unsigned int> pair)
{
	storedMaterials.push_back(pair);
};

void ModelMgr::Reset(CVehicle *pVeh)
{
	materials.erase(pVeh->m_nModelIndex);
}

void ModelMgr::RestoreMaterials()
{
	for (auto &p : storedMaterials)
	{
		*p.first = p.second;
	}
	storedMaterials.clear();
};

void ModelMgr::OnRender(CVehicle *vehicle)
{
	if (!renders.empty())
	{
		for (auto e : renders)
		{
			e(vehicle);
		}
	}
};

RpMaterial *ModelMgr::SetEditableMaterialsCB(RpMaterial *material, void *data)
{
	tRestoreEntry **ppEntries = reinterpret_cast<tRestoreEntry **>(data);
	CRGBA color = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(material));
	color.a = 255;

	if (CVehicleModelInfo::ms_pRemapTexture && RpMaterialGetTexture(material) && RwTextureGetName(RpMaterialGetTexture(material))[0] == '#')
	{
		(*ppEntries)->m_pAddress = &material->texture;
		(*ppEntries)->m_pValue = material->texture;
		(*ppEntries)++;
		material->texture = CVehicleModelInfo::ms_pRemapTexture;
	}

	if (material->texture == CVehicleModelInfo::ms_pLightsTexture)
	{
		int iLightIndex = -1;
		if (color == VEHCOL_HEADLIGHT_LEFT) {
			iLightIndex = 0;
		}
		else if (color == VEHCOL_HEADLIGHT_RIGHT) {
			iLightIndex = 1;
		}
		else if (color == VEHCOL_TAILLIGHT_LEFT) {
			iLightIndex = 2;
		}
		else if (color == VEHCOL_TAILLIGHT_RIGHT) {
			iLightIndex = 3;
		}

		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = 255;
		RpMaterialGetColor(material)->green = 255;
		RpMaterialGetColor(material)->blue = 255;

		if (iLightIndex != -1 && CVehicleModelInfo::ms_lightsOn[iLightIndex])
		{
			(*ppEntries)->m_pAddress = &material->texture;
			(*ppEntries)->m_pValue = material->texture;
			(*ppEntries)++;

			(*ppEntries)->m_pAddress = RpMaterialGetSurfaceProperties(material);
			(*ppEntries)->m_pValue = *(void **)RpMaterialGetSurfaceProperties(material);
			(*ppEntries)++;

			material->texture = CVehicleModelInfo::ms_pLightsOnTexture;
			RpMaterialSetSurfaceProperties(material, &gLightSurfProps);
		}
	}
	else
	{
		int iColorIndex;
		if (color == VEHCOL_PRIMARY)
		{
			iColorIndex = CVehicleModelInfo::ms_currentCol[0];
		}
		else if (color == VEHCOL_SECONDARY)
		{
			iColorIndex = CVehicleModelInfo::ms_currentCol[1];
		}
		else if (color == VEHCOL_TERTIARY)
		{
			iColorIndex = CVehicleModelInfo::ms_currentCol[2];
		}
		else if (color == VEHCOL_QUATARNARY)
		{
			iColorIndex = CVehicleModelInfo::ms_currentCol[3];
		}
		else
		{
			return material;
		}

		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		auto &color = CVehicleModelInfo::ms_vehicleColourTable[iColorIndex];
		RpMaterialGetColor(material)->red = color.r;
		RpMaterialGetColor(material)->green = color.g;
		RpMaterialGetColor(material)->blue = color.b;
	}

	return material;
}