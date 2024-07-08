#include "pch.h"
#include "materials.h"

VehicleMaterial::VehicleMaterial(RpMaterial* material) {
	Material = material;

	Texture = material->texture;
	TextureActive = material->texture;

	Color = { material->color.red, material->color.green, material->color.blue, material->color.alpha };

	std::string name = std::string(Texture->name);

	if (RwTexture* newTexture = RwTexDictionaryFindNamedTexture(material->texture->dict, std::string(name + "on").c_str()))
		TextureActive = newTexture;
	else if (RwTexture* newTexture = RwTexDictionaryFindNamedTexture(material->texture->dict, std::string(name + "_on").c_str()))
		TextureActive = newTexture;
};

void VehicleMaterials::Register(VehicleMaterialFunction function) {
	functions.push_back(function);
};

void VehicleMaterials::RegisterRender(VehicleMaterialRender render) {
	renders.push_back(render);
};

void VehicleMaterials::RegisterDummy(VehicleDummyFunction function) {
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

			for (std::vector<VehicleMaterialFunction>::iterator function = functions.begin(); function != functions.end(); ++function)
				(*function)(currentVehicle, material);

			materials[currentVehicle->m_nModelIndex][material] = true;

			return material;
		}, atomic);

		return atomic;
	}, (void*)((uint32_t)(0)));

	if (!dummies.contains(currentVehicle->m_nModelIndex) || dummies[currentVehicle->m_nModelIndex] == false) {
		dummies[currentVehicle->m_nModelIndex] = true;
	}


	//(RwFrame*)vehicle->m_pRwClump->object.parent

	VehicleMaterials::findDummies(vehicle, (RwFrame*)vehicle->m_pRwClump->object.parent);
};

void VehicleMaterials::findDummies(CVehicle* vehicle, RwFrame* frame, bool parent) {
	if (!frame)
		return;

	const std::string name = GetFrameNodeName(frame);

	//PluginMultiplayer::AddChatMessage(std::string(name + ": has child? " + ((frame->child)?("yes"):("no"))).c_str());

	if (RwFrame* nextFrame = frame->child)
		findDummies(vehicle, nextFrame, (RwFrameGetParent(frame))?(true):(false));

	if (RwFrame* nextFrame = frame->next)
		findDummies(vehicle, nextFrame, parent);

	if (frames[currentVehicle->m_nModelIndex].contains(frame))
		return;

	frames[currentVehicle->m_nModelIndex][frame] = true;

	for (std::vector<VehicleDummyFunction>::iterator function = dummy.begin(); function != dummy.end(); ++function)
		(*function)(currentVehicle, frame, name, parent);

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

	for (std::vector<VehicleMaterialRender>::iterator render = renders.begin(); render != renders.end(); ++render)
		(*render)(vehicle, index);
};
