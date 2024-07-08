#include "pch.h"
#include "paintjobs.h"
#include <plugin.h>
#include <CPools.h>

void VehiclePaintjobs::Read(int model, nlohmann::json json) {
	for (nlohmann::json::iterator paintjob = json.begin(); paintjob != json.end(); ++paintjob) {
		int _paintjob = std::stoi(paintjob.key());

		nlohmann::json identifiers = paintjob.value().find("identifiers").value();

		for (nlohmann::json::iterator variable = identifiers.begin(); variable != identifiers.end(); ++variable) {
			modelPaintjobs[model][variable.value()] = _paintjob;
		}
	}
};

void VehiclePaintjobs::OnVehicleSetModel(CVehicle* vehicle) {
	vehiclePaintjob[CPools::ms_pVehiclePool->GetIndex(vehicle)] = false;
};

void VehiclePaintjobs::OnVehicleRender(CVehicle* vehicle) {
	int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

	if (vehiclePaintjob[index] == true)
		return;

	vehiclePaintjob[index] = true;

	if (!modelPaintjobs.count(vehicle->m_nModelIndex))
		return;

	int identifier = (int)vehicle; // TODO
	if (!modelPaintjobs[vehicle->m_nModelIndex].count((identifier)))
		return;

	vehicle->SetRemap(modelPaintjobs[vehicle->m_nModelIndex][identifier]);
};

void VehiclePaintjobs::Initialize() {
	std::string path = MOD_DATA_PATH("paintjobs/");
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            int model = std::stoi(filename.substr(0, filename.find(".")));

            gLogger->info("Reading paintjob configuration from {}", entry.path().string());

            std::ifstream stream(entry.path());

            nlohmann::json json;
            stream >> json;
            stream.close();

            VehiclePaintjobs::Read(model, json);
        }
    }
}