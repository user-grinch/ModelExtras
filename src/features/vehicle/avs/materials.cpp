#include "pch.h"
#include "materials.h"
#include <CTxdStore.h>

VehicleMaterial::VehicleMaterial(RpMaterial* material) {
	Material = material;

	Texture = material->texture;
	TextureActive = material->texture;

	Color = { material->color.red, material->color.green, material->color.blue, material->color.alpha };

	std::string name = std::string(Texture->name);

	RwTexture *pTexture = RwTexDictionaryFindNamedTexture(material->texture->dict, std::string(name + "on").c_str());
	if (!pTexture) {
		pTexture = RwTexDictionaryFindNamedTexture(material->texture->dict, std::string(name + "_on").c_str());
	}

	if (!pTexture) {
		pTexture = RwTexDictionaryFindNamedTexture(material->texture->dict, "vehiclelightson128");
	}

	if (!pTexture) {
		int slot = CTxdStore::FindTxdSlot("vehicle");
		if (slot < 0) {
		    slot = CTxdStore::AddTxdSlot("vehicle");
			CTxdStore::LoadTxd(slot, "vehicle");
		}
		CTxdStore::SetCurrentTxd(slot);
		pTexture = RwTexDictionaryFindNamedTexture(RwTexDictionaryGetCurrent(), "vehiclelightson128");
		CTxdStore::PopCurrentTxd();
	}

	if (pTexture) {
		TextureActive = pTexture;
	}
};

void VehicleMaterials::Register(std::function<RpMaterial*(CVehicle*, RpMaterial*)> function) {
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

			if (materials[currentVehicle->m_nModelIndex].contains(material))
				return material;

			for (auto& e: functions)
				e(currentVehicle, material);

			materials[currentVehicle->m_nModelIndex][material] = true;

			return material;
		}, atomic);

		return atomic;
	}, nullptr);

	if (!dummies.contains(currentVehicle->m_nModelIndex) || dummies[currentVehicle->m_nModelIndex] == false) {
		dummies[currentVehicle->m_nModelIndex] = true;
	}

	VehicleMaterials::findDummies(vehicle, (RwFrame*)vehicle->m_pRwClump->object.parent);
};

void VehicleMaterials::findDummies(CVehicle* vehicle, RwFrame* frame, bool parent) {
	if (!frame)
		return;

	const std::string name = GetFrameNodeName(frame);

	if (RwFrame* nextFrame = frame->child)
		findDummies(vehicle, nextFrame, (RwFrameGetParent(frame))?(true):(false));

	if (RwFrame* nextFrame = frame->next)
		findDummies(vehicle, nextFrame, parent);

	if (frames[currentVehicle->m_nModelIndex].contains(frame))
		return;

	frames[currentVehicle->m_nModelIndex][frame] = true;

	for (auto e: dummy)
		e(currentVehicle, frame, name, parent);

	return;
};

void VehicleMaterials::StoreMaterial(std::pair<unsigned int*, unsigned int> pair) {
	storedMaterials.push_back(pair);
};

void VehicleMaterials::RestoreMaterials() {
	for (auto& p : storedMaterials)
		*p.first = p.second;

	storedMaterials.clear();
};

void VehicleMaterials::OnRender(CVehicle* vehicle) {
	if (renders.size() == 0)
		return;
	
	int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

	for (auto e: renders)
		e(vehicle);
};