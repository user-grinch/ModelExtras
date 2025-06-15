#include "pch.h"
#include "modelinfomgr.h"
#include <CTxdStore.h>
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

extern int GetSirenIndex(CVehicle *pVeh, RpMaterial *pMat);
extern int GetStrobeIndex(CVehicle *pVeh, RpMaterial *pMat);

static CVehicle *pCurVeh = nullptr;
void ModelInfoMgr::Initialize() {
	// Nop frame collasping
    plugin::patch::Nop(0x4C8E53, 5);
    plugin::patch::Nop(0x4C8F6E, 5);

	patch::ReplaceFunctionCall(0x5532A9, ModelInfoMgr::SetupRender);
    patch::ReplaceFunction(0x4C8220, ModelInfoMgr::SetEditableMaterialsCB);

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
	m_LightStatus[pVeh][type] = true;
}

void ModelInfoMgr::EnableSirenMaterial(CVehicle *pVeh, int idx) {
	m_SirenStatus[pVeh][idx] = true;
}

void ModelInfoMgr::EnableStrobeMaterial(CVehicle *pVeh, int idx) {
	m_StrobeStatus[pVeh][idx] = true;
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
	ptr->SetupRender();
	for (int i = 0; i < eLightType::TotalLight; i++) {
		m_LightStatus[ptr][i] = false;
	}

	for (int i = 0; i < MAX_LIGHTS; i++) {
		m_SirenStatus[ptr][i] = false;
	}

	for (int i = 0; i < MAX_LIGHTS; i++) {
		m_StrobeStatus[ptr][i] = false;
	}
}

RwSurfaceProperties &gLightSurfPropsOn = *(RwSurfaceProperties *)0x8A645C;
RwSurfaceProperties gLightSurfProps = {1.0, 0.0, 0.0};

struct tRestoreEntry
{
	void *m_pAddress;
	void *m_pValue;
};

CRGBA ModelInfoMgr::FetchMaterialCol(CVehicle *pVeh, RpMaterial *pMat) {
	CRGBA col = DEFAULT_MAT_COL;
	for (auto& e : matColProviders) {
		col = e(pVeh, pMat);
		if (col != DEFAULT_MAT_COL) {
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
	if (!material || !material->texture) {
		return material;
	}

	tRestoreEntry **ppEntries = reinterpret_cast<tRestoreEntry **>(data);
	CRGBA matCol = *reinterpret_cast<CRGBA *>(RpMaterialGetColor(material));
	matCol.a = 255;

	if (CVehicleModelInfo::ms_pRemapTexture && RpMaterialGetTexture(material) && RwTextureGetName(RpMaterialGetTexture(material))[0] == '#')
	{
		(*ppEntries)->m_pAddress = &material->texture;
		(*ppEntries)->m_pValue = material->texture;
		(*ppEntries)++;
		material->texture = CVehicleModelInfo::ms_pRemapTexture;
	}

	DirtFx::ProcessTextures(pCurVeh, material);

	eLightType iLightIndex = FetchMaterialType(pCurVeh, material);
	if (iLightIndex != eLightType::UnknownLight)
	{
		bool lightOn = false;
		if (iLightIndex == eLightType::SirenLight) {
			lightOn = m_SirenStatus[pCurVeh][GetSirenIndex(pCurVeh, material)];
		} else if (iLightIndex == eLightType::StrobeLight){
			lightOn = m_StrobeStatus[pCurVeh][GetStrobeIndex(pCurVeh, material)];
		} else if (iLightIndex != eLightType::UnknownLight) {
			lightOn = m_LightStatus[pCurVeh][iLightIndex];
		}
		
		CRGBA enabledCol = FetchMaterialCol(pCurVeh, material);
		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = enabledCol.r;
		RpMaterialGetColor(material)->green = enabledCol.g;
		RpMaterialGetColor(material)->blue = enabledCol.b;

		if (lightOn)
		{
			(*ppEntries)->m_pAddress = &material->texture;
			(*ppEntries)->m_pValue = material->texture;
			(*ppEntries)++;

			(*ppEntries)->m_pAddress = RpMaterialGetSurfaceProperties(material);
			(*ppEntries)->m_pValue = *(void **)RpMaterialGetSurfaceProperties(material);
			(*ppEntries)++;

			if (material->texture == CVehicleModelInfo::ms_pLightsTexture) {
				material->texture = CVehicleModelInfo::ms_pLightsOnTexture;
			} else {
				RwTexture *pTex = TextureMgr::FindTextureInDict(material, material->texture->dict);
				if (pTex) {
					material->texture = pTex;
				} else{
					LOG_VERBOSE("Expected an 'on' texture for {} but none found", material->texture->name);
				}
			}
			RpMaterialSetSurfaceProperties(material, &gLightSurfPropsOn);
		} else {
			RpMaterialGetColor(material)->red = DEFAULT_MAT_COL.r;
			RpMaterialGetColor(material)->green = DEFAULT_MAT_COL.g;
			RpMaterialGetColor(material)->blue = DEFAULT_MAT_COL.b;
			RpMaterialSetSurfaceProperties(material, &gLightSurfProps);
		}
	}
	else
	{
		CRGBA col = IVFCarcols::GetColor(pCurVeh, material, matCol);
		
		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = col.r;
		RpMaterialGetColor(material)->green = col.g;
		RpMaterialGetColor(material)->blue = col.b;
	}

	return material;
}