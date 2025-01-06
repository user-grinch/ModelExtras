#include "pch.h"
#include "materials.h"
#include <CTxdStore.h>
#include <RenderWare.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>

VehicleMaterial::VehicleMaterial(RpMaterial* material, eDummyPos pos) {
	if (!material) return;

	Material = material;
	if (!material->texture) return;

	Texture = material->texture;
	TextureActive = material->texture;
	Pos = pos;
	Color = { material->color.red, material->color.green, material->color.blue, material->color.alpha };

	const std::string baseName = material->texture->name;
	const std::vector<std::string> texNames = {
		baseName + "on",
		baseName + "_on",
		"sirenlighton",
		"sirenlight_on",
		"vehiclelightson128"
	};

	RwTexture* pTexture = nullptr;

	for (const auto& name : texNames) {
		pTexture = RwTexDictionaryFindNamedTexture(material->texture->dict, name.c_str());
		if (pTexture) break;
	}

	if (!pTexture) {
		int slot = CTxdStore::FindTxdSlot("vehicle");
		if (slot < 0) {
			slot = CTxdStore::AddTxdSlot("vehicle");
			CTxdStore::LoadTxd(slot, "vehicle");
		}

		CTxdStore::SetCurrentTxd(slot);
		if (auto* currentDict = RwTexDictionaryGetCurrent()) {
			pTexture = RwTexDictionaryFindNamedTexture(currentDict, "vehiclelightson128");
		}
		CTxdStore::PopCurrentTxd();
	}

	if (pTexture) {
		TextureActive = pTexture;
	}
}


void VehicleMaterials::Register(std::function<RpMaterial* (CVehicle*, RpMaterial*)> function) {
	functions.push_back(function);
};

void VehicleMaterials::RegisterRender(std::function<void(CVehicle*)> render) {
	renders.push_back(render);
};

void VehicleMaterials::RegisterDummy(std::function<void(CVehicle*, RwFrame*, std::string, bool)> function) {
	dummy.push_back(function);
};

void VehicleMaterials::OnModelSet(CVehicle* vehicle, int model) {
	currentVehicle = vehicle;

	RpClumpForAllAtomics(vehicle->m_pRwClump, [](RpAtomic* atomic, void* data) {
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

			for (auto& e : functions)
				e(currentVehicle, material);

			materials[currentVehicle->m_nModelIndex][material] = true;

			return material;
		}, atomic);

		return atomic;
	}, nullptr);

	VehicleMaterials::FindDummies(vehicle, (RwFrame*)vehicle->m_pRwClump->object.parent);
};

void VehicleMaterials::FindDummies(CVehicle* vehicle, RwFrame* frame, bool parent) {
	if (frame) {
		const std::string name = GetFrameNodeName(frame);

		if (RwFrame* nextFrame = frame->child) {
			FindDummies(vehicle, nextFrame, (RwFrameGetParent(frame)) ? (true) : (false));
		}

		if (RwFrame* nextFrame = frame->next) {
			FindDummies(vehicle, nextFrame, parent);
		}

		for (auto e : dummy) {
			e(currentVehicle, frame, name, parent);
		}
	}
};

void VehicleMaterials::StoreMaterial(std::pair<unsigned int*, unsigned int> pair) {
	storedMaterials.push_back(pair);
};

void VehicleMaterials::RestoreMaterials() {
	for (auto& p : storedMaterials) {
		*p.first = p.second;
	}
	storedMaterials.clear();
};

void VehicleMaterials::OnRender(CVehicle* vehicle) {
	if (!renders.empty()) {
		for (auto e : renders) {
			e(vehicle);
		}
	}
};