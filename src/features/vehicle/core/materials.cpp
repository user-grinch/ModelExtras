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

void ModelInfoMgr::EnableLight(CVehicle *pVeh, eLightType type) {
	m_LightStatus[pVeh][type] = true;
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
		int iLightIndex = eLightType::UnknownLight;

		for (auto& e : materials) {
			eLightType type = e(color);
			if (type != eLightType::UnknownLight) {
				iLightIndex = type;
				break;
			}
		}

		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = 255;
		RpMaterialGetColor(material)->green = 255;
		RpMaterialGetColor(material)->blue = 255;

		if (iLightIndex != eLightType::UnknownLight && m_LightStatus[pCurVeh][iLightIndex])
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
		CRGBA col = IVFCarcols::GetColor(pCurVeh, material, color);

		(*ppEntries)->m_pAddress = RpMaterialGetColor(material);
		(*ppEntries)->m_pValue = *reinterpret_cast<void **>(RpMaterialGetColor(material));
		(*ppEntries)++;

		RpMaterialGetColor(material)->red = col.r;
		RpMaterialGetColor(material)->green = col.g;
		RpMaterialGetColor(material)->blue = col.b;
	}

	return material;
}