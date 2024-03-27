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
}

void VehicleMaterials::Register(std::function<RpMaterial*(CVehicle*, RpMaterial*, bool*)> function) {
    functions.push_back(function);
}

void VehicleMaterials::RegisterDummy(std::function<void(CVehicle*, RwFrame*, std::string, bool)> function) {
    dummy.push_back(function);
}

void VehicleMaterials::RegisterRender(std::function<void(CVehicle*)> render) {
    renders.push_back(render);
}

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

            for (auto& function : functions) {
                bool clearMat = false;
                function(currentVehicle, material, &clearMat);

                if (clearMat) {
                    registeredMats[reinterpret_cast<CVehicle*>(data)->m_nModelIndex].push_back(material->texture);
                }
            }

            materials[currentVehicle->m_nModelIndex][material] = true;

            return material;
        }, data);

        RpGeometryForAllMaterials(atomic->geometry, [](RpMaterial* material, void* data) {
            if (!material || !material->texture)
                return material;

            for (auto e: registeredMats[reinterpret_cast<CVehicle*>(data)->m_nModelIndex]) {
                if (material->texture == e) {
                    material->color.red = material->color.green = material->color.blue = 255;
                }
            }

            return material;
        }, data);

        return atomic;
    }, vehicle);

    if (!dummies.contains(currentVehicle->m_nModelIndex) || !dummies[currentVehicle->m_nModelIndex]) {
        dummies[currentVehicle->m_nModelIndex] = true;
    }

    findDummies(vehicle, reinterpret_cast<RwFrame*>(vehicle->m_pRwClump->object.parent));
}

void VehicleMaterials::findDummies(CVehicle* vehicle, RwFrame* frame, bool parent) {
    if (!frame)
        return;

    const std::string name = GetFrameNodeName(frame);

    if (RwFrame* nextFrame = frame->child)
        findDummies(vehicle, nextFrame, RwFrameGetParent(frame) ? true : false);

    if (RwFrame* nextFrame = frame->next)
        findDummies(vehicle, nextFrame, parent);

    // if (frames[currentVehicle->m_nModelIndex].contains(frame))
    //     return;

    frames[currentVehicle->m_nModelIndex][frame] = true;

    for (auto& function : dummy)
        function(currentVehicle, frame, name, parent);
}

void VehicleMaterials::StoreMaterial(std::pair<unsigned int*, unsigned int> pair) {
    storedMaterials.push_back(pair);
}

void VehicleMaterials::RestoreMaterials() {
    for (auto& p : storedMaterials)
        *p.first = p.second;

    storedMaterials.clear();
}

void VehicleMaterials::OnRender(CVehicle* pVeh) {
    if (renders.empty())
        return;

    for (auto& render : renders)
        render(pVeh);
}
