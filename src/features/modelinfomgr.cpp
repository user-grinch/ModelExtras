#include "pch.h"
#include "modelinfomgr.h"
#include <CTxdStore.h>
#include <CCamera.h>
#include <RenderWare.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include "texmgr.h"
#include "colors.h"
#include <NodeName.h>
#include "../carcols.h"
#include "meevents.h"
#include "dirtfx.h"
#include "plate.h"	

extern int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
extern int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat);

static CVehicle *pCurVeh = nullptr;
RwSurfaceProperties& gLightSurfProps = *(RwSurfaceProperties*)0x8A645C;
RwSurfaceProperties gLightSurfPropsOff = {0.45, 0.0, 0.0};

void ModelInfoMgr::Initialize() {
	// Nop frame collasping
    plugin::patch::Nop(0x4C8E53, 5);
    plugin::patch::Nop(0x4C8F6E, 5);

	patch::ReplaceFunctionCall(0x5532A9, ModelInfoMgr::SetupRender);
    patch::ReplaceFunction(0x4C8220, ModelInfoMgr::SetEditableMaterialsCB);

	Events::initScriptsEvent += []() {
		gLightSurfProps.ambient = gConfig.ReadFloat("VISUAL", "MaterialAmbientOn", gLightSurfProps.ambient);
		gLightSurfPropsOff.ambient = gConfig.ReadFloat("VISUAL", "MaterialAmbientOff", gLightSurfPropsOff.ambient);
	};

	MEEvents::vehRenderEvent.before += [](CVehicle *pVeh)
    {
        ModelInfoMgr::OnRender(pVeh);
    };

    MEEvents::heliRenderEvent.after += [](CVehicle *pVeh)
    {
		if (CModelInfo::IsHeliModel(pVeh->m_nModelIndex))
        {
            ModelInfoMgr::OnRender(pVeh);
        }
    };

    Events::vehicleSetModelEvent.after += [](CVehicle *pVeh, int model)
    {
        ModelInfoMgr::FindDummies(pVeh, (RwFrame *)pVeh->m_pRwClump->object.parent);
    };
}

void ModelInfoMgr::RegisterRender(RenderCallback_t render)
{
	renders.push_back(render);
};

void ModelInfoMgr::RegisterDummy(DummyCallback_t function)
{
	dummy.push_back(function);
};

void ModelInfoMgr::EnableLightMaterial(CVehicle *pVeh, eLightType type) {
	auto& data = m_VehData.Get(pVeh);
	data.m_LightStatus[type] = true;
}

void ModelInfoMgr::EnableSirenMaterial(CVehicle *pVeh, int idx) {
	auto& data = m_VehData.Get(pVeh);
	data.m_SirenStatus[idx] = true;
}

void ModelInfoMgr::EnableStrobeMaterial(CVehicle *pVeh, int idx) {
	auto& data = m_VehData.Get(pVeh);
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

		for (auto e : dummy)
		{
			e(vehicle, frame);
		}
	}
};

void ModelInfoMgr::Reload(CVehicle *pVeh) {
	if (pVeh->m_pRwClump) {
		RwFrame *frame = reinterpret_cast<RwFrame*>(pVeh->m_pRwClump->object.parent);
		FindDummies(pVeh, frame);
	}
}

void ModelInfoMgr::OnRender(CVehicle *vehicle)
{
	if (!renders.empty())
	{
		for (auto e : renders)
		{
			e(vehicle);
		}
	}
}

void ModelInfoMgr::RegisterMaterial(MaterialCallback_t mat) {
	materials.push_back(mat);
}

void ModelInfoMgr::RegisterMaterialColProvider(MaterialColProviderCallback_t mat) {
	matColProviders.push_back(mat);
}

void ModelInfoMgr::SetupRender(CVehicle *ptr)
{
	pCurVeh = ptr;
	auto& data = m_VehData.Get(pCurVeh);
	ptr->SetupRender();
	for (int i = 0; i < eLightType::TotalLight; i++) {
		data.m_LightStatus[i] = false;
	}

	for (int i = 0; i < MAX_LIGHTS; i++) {
		data.m_SirenStatus[i] = false;
	}

	for (int i = 0; i < MAX_LIGHTS; i++) {
		data.m_StrobeStatus[i] = false;
	}
}

struct tRestoreEntry
{
	void *m_pAddress;
	void *m_pValue;
};

MatStateColor ModelInfoMgr::FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat, eLightType type) {
	MatStateColor col = {DEFAULT_MAT_COL, DEFAULT_MAT_COL};
	for (auto& e : matColProviders) {
		col = e(pVeh, pMat, type);
		if (col.on != DEFAULT_MAT_COL || col.off != DEFAULT_MAT_COL) {
			break;
		}
	}
	return col;
}

eLightType ModelInfoMgr::FetchMaterialType(CVehicle *pVeh, RpMaterial *pMat) {
	eLightType matType = eLightType::UnknownLight;

	for (auto& e : materials) {
		eLightType type = e(pVeh, pMat);
		if (type != eLightType::UnknownLight) {
			matType = type;
			break;
		}
	}
	return matType;
}

RpMaterial *ModelInfoMgr::SetEditableMaterialsCB(RpMaterial *material, void *data)
{
	if (!material) {
		return material;
	}

	tRestoreEntry **ppEntries = reinterpret_cast<tRestoreEntry **>(data);
	if (material->texture) {
		bool isRemapTex = RwTextureGetName(RpMaterialGetTexture(material))[0] == '#';
		if (isRemapTex) {
			if (CVehicleModelInfo::ms_pRemapTexture)
			{
				(*ppEntries)->m_pAddress = &material->texture;
				(*ppEntries)->m_pValue = material->texture;
				(*ppEntries)++;
				material->texture = CVehicleModelInfo::ms_pRemapTexture;
			}
		} else {
			DirtFx::ProcessTextures(pCurVeh, material);
			LicensePlate::ProcessTextures(pCurVeh, material);
		}
	}

	eLightType iLightIndex = FetchMaterialType(pCurVeh, material);
	if (iLightIndex != eLightType::UnknownLight)
	{
		auto& data = m_VehData.Get(pCurVeh);
		bool lightOn = false;
		data.m_MatAvail[iLightIndex] = true;

		if (iLightIndex == eLightType::SirenLight) {
			lightOn = data.m_SirenStatus[GetSirenIndex(pCurVeh, material)];
		} else if (iLightIndex == eLightType::StrobeLight){
			lightOn = data.m_StrobeStatus[GetStrobeIndex(pCurVeh, material)];
		} else if (iLightIndex != eLightType::UnknownLight) {
			lightOn = data.m_LightStatus[iLightIndex];
		}
		
		MatStateColor matCol = FetchMaterialCol(pCurVeh, material, iLightIndex);
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

			if (material->texture) {
				if (material->texture == CVehicleModelInfo::ms_pLightsTexture) {
					material->texture = CVehicleModelInfo::ms_pLightsOnTexture;
				} else {
					RwTexture *pTex = TextureMgr::FindOnTextureInDict(material, material->texture->dict);
					if (pTex) {
						material->texture = pTex;
					} else{
						LOG_VERBOSE("Expected an 'on' texture for {} but none found", material->texture->name);
					}
				}
			}
			material->surfaceProps.ambient = gLightSurfProps.ambient;
		} else {
			RpMaterialGetColor(material)->red = matCol.off.r;
			RpMaterialGetColor(material)->green = matCol.off.g;
			RpMaterialGetColor(material)->blue = matCol.off.b;
			material->surfaceProps.ambient = gLightSurfPropsOff.ambient;
		}
	}
	else
	{
		CRGBA col = {255, 255, 255, 255};
		if (IVFCarcols::GetColor(pCurVeh, material, col)) {
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

bool ModelInfoMgr::IsMaterialAvailable(CVehicle *pVeh, eLightType type) {
	auto& data = m_VehData.Get(pVeh);
	return data.m_MatAvail[type];
}