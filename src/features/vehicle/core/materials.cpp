#include "pch.h"
#include "materials.h"
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
#include "sirens.h"

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

RwSurfaceProperties &gLightSurfProps = *(RwSurfaceProperties *)0x8A645C;
struct tRestoreEntry
{
	void *m_pAddress;
	void *m_pValue;
};

RpMaterial *ModelInfoMgr::SetEditableMaterialsCB(RpMaterial *material, void *data)
{
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

	eLightType iLightIndex = eLightType::UnknownLight;

	for (auto& e : materials) {
		eLightType type = e(pCurVeh, material);
		if (type != eLightType::UnknownLight) {
			iLightIndex = type;
			break;
		}
	}

	if (material->texture == CVehicleModelInfo::ms_pLightsTexture || iLightIndex != eLightType::UnknownLight)
	{
		bool lightStatus = false;

		if (iLightIndex == eLightType::SirenLight) {
			lightStatus = m_SirenStatus[pCurVeh][GetSirenIndex(pCurVeh, material)];
		} else if (iLightIndex == eLightType::StrobeLight){
			lightStatus = m_StrobeStatus[pCurVeh][GetStrobeIndex(pCurVeh, material)];
		} else if (iLightIndex != eLightType::UnknownLight) {
			lightStatus = m_LightStatus[pCurVeh][iLightIndex];
		}


		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = 255;
		RpMaterialGetColor(material)->green = 255;
		RpMaterialGetColor(material)->blue = 255;

		if (iLightIndex != eLightType::UnknownLight && lightStatus)
		{
			(*ppEntries)->m_pAddress = &material->texture;
			(*ppEntries)->m_pValue = material->texture;
			(*ppEntries)++;

			(*ppEntries)->m_pAddress = RpMaterialGetSurfaceProperties(material);
			(*ppEntries)->m_pValue = *(void **)RpMaterialGetSurfaceProperties(material);
			(*ppEntries)++;

			if (iLightIndex == eLightType::SirenLight) {
				RwTexture *pTex = TextureMgr::FindTextureInDict(material, material->texture->dict);
				if (pTex) {
					material->texture = pTex;
				}
			} else {
				material->texture = CVehicleModelInfo::ms_pLightsOnTexture;
			}
			RpMaterialSetSurfaceProperties(material, &gLightSurfProps);
		}
	}
	else
	{
		CRGBA col = {255, 255, 255, 255};
		if (!CModelInfo::IsHeliModel(pCurVeh->m_nModelIndex)) {
			col = IVFCarcols::GetColor(pCurVeh, material, matCol);
		} 
		
		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = col.r;
		RpMaterialGetColor(material)->green = col.g;
		RpMaterialGetColor(material)->blue = col.b;
	}

	return material;
}