#include "pch.h"
#include "sirens.h"
#include <common.h>
#include "avs/common.h"
#include <CShadows.h>

int ImVehFt_ReadColor(std::string input) {
    if (input.length() == 3)
        return std::stoi(input);

    std::istringstream stream(input);
    int color;
    stream >> std::hex >> color;
    return color;
};

bool VehicleSiren::GetSirenState() {
	return (Mute == false) ? (vehicle->m_nVehicleFlags.bSirenOrAlarm) : (true);
};

char __fastcall VehicleSirens::hkUsesSiren(CVehicle *ptr) {
	if(VehicleSirens::modelData.contains(ptr->m_nModelIndex)){
		ptr->m_vehicleAudio.m_bModelWithSiren = true;
		return true;
	}
	return ptr->IsLawEnforcementVehicle();
}

VehicleSirenMaterial::VehicleSirenMaterial(std::string state, int material, nlohmann::json json) {
	if (json.size() == 0) {
		gLogger->error("Model {} siren configuration exception!", VehicleSirens::CurrentModel);
		return;
	}

	if (json.contains("reference")) {
		std::vector<std::string> references;

		if (json["reference"].is_string())
			references.push_back(json["reference"]);
		else if (json["reference"].is_array()) {
			for (nlohmann::json::iterator reference = json["reference"].begin(); reference != json["reference"].end(); ++reference) {
				if (reference.value().is_string())
					references.push_back(reference.value());
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference array property is not an string!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference property is not an array or string!");

		if (references.size() != 0) {
			for (std::vector<std::string>::iterator ref = references.begin(); ref != references.end(); ++ref) {
				if (VehicleSirenData::References.contains((*ref))) {
					for (nlohmann::json::iterator reference = VehicleSirenData::References[(*ref)].begin(); reference != VehicleSirenData::References[(*ref)].end(); ++reference) {
						if (json.contains(reference.key())) {
							if (json[reference.key()].is_object()) {
								nlohmann::json object = reference.value();

								for (nlohmann::json::iterator objectReference = object.begin(); objectReference != object.end(); ++objectReference) {
									if (json[reference.key()].contains(objectReference.key()))
										continue;

									json[reference.key()][objectReference.key()] = objectReference.value();
								}
							}

							continue;
						}

						json[reference.key()] = reference.value();
					}
				}
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference array property '" + (*ref) + "' is not an recognized reference!");
			}
		}
	}

	if (json.contains("size")) {
		if(json["size"].is_number())
			Size = json["size"];
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", size property is not an acceptable number!");
	}

	if (json.contains("diffuse")) {
		if(json["diffuse"].is_boolean())
			Diffuse.Color = json["diffuse"];
		else if (json["diffuse"].is_object()) {
			if (json["diffuse"].contains("color")) {
				if (json["diffuse"]["color"].is_boolean())
					Diffuse.Color = json["diffuse"]["color"];
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property color is not an boolean!");
			}

			if (json["diffuse"].contains("transparent")) {
				if (json["diffuse"]["transparent"].is_boolean())
					Diffuse.Transparent = json["diffuse"]["transparent"];
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property transparent is not an boolean!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property is not an boolean or object!");
	}

	if (json.contains("radius")) {
		if(json["radius"].is_number())
			Radius = json["radius"];
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", radius property is not an acceptable number!");
	}

	if (json.contains("color")) {
		if (json["color"].is_string() && VehicleSirenData::ReferenceColors.contains(json["color"]))
			json["color"] = VehicleSirenData::ReferenceColors[json["color"]];

		if (json["color"].is_object()) {
			if (json["color"].contains("red"))
				Color.red = json["color"]["red"];

			if (json["color"].contains("green"))
				Color.green = json["color"]["green"];

			if (json["color"].contains("blue"))
				Color.blue = json["color"]["blue"];

			if (json["color"].contains("alpha"))
				Color.alpha = json["color"]["alpha"];

			DefaultColor = { Color.red, Color.green, Color.blue, Color.alpha };
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", color property is not an object!");
	}

	if (json.contains("state")) {
		if (json["state"].is_boolean() || json["state"].is_number()) {
			bool state = (json["state"].is_boolean()) ? ((bool)json["state"]) : (((json["state"] == 0) ? (false) : (true)));

			State = state;

			StateDefault = state;
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", state property is not an boolean or number!");
	}

	if (json.contains("colors")) {
		if (json["colors"].is_array()) {
			for (nlohmann::json::iterator pattern = json["colors"].begin(); pattern != json["colors"].end(); ++pattern) {
				nlohmann::json value = pattern.value();

				if (!value.is_array()) {
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property is not an array!");

					continue;
				}

				if (value.size() != 2) {
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property is not an array with a pair value of delay and color!");

					continue;
				}

				uint64_t time = value[0];

				RwRGBA color;

				if (value[1].is_string() && VehicleSirenData::ReferenceColors.contains(value[1]))
					value[1] = VehicleSirenData::ReferenceColors[value[1]];

				if (value[1].is_object()) {
					if (value[1].contains("red"))
						color.red = value[1]["red"];

					if (value[1].contains("green"))
						color.green = value[1]["green"];

					if (value[1].contains("blue"))
						color.blue = value[1]["blue"];

					if (value[1].contains("alpha"))
						color.alpha = value[1]["alpha"];

					Colors.push_back(std::make_pair(time, color));

					ColorTotal += time;
				}
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property does not contain an array!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors property is not an array!");
	}

	if (json.contains("pattern")) {
		if (json["pattern"].is_array()) {
			for (nlohmann::json::iterator pattern = json["pattern"].begin(); pattern != json["pattern"].end(); ++pattern) {
				nlohmann::json value = pattern.value();

				if (value.is_array()) {
					if (value.size() < 2) {
						gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern array property is an array but does not contain an iteration and pattern setting!");

						continue;
					}

					for (int count = 0; count < value[0]; count++) {
						for (nlohmann::json::iterator vector = ++value.begin(); vector != value.end(); ++vector) {
							int time = vector.value();

							Pattern.push_back(time);

							PatternTotal += time;
						}
					}

					continue;
				}
				else if (value.is_number()) {
					int time = pattern.value();

					Pattern.push_back(time);

					PatternTotal += time;
				}
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern array property is not an array or number!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern property is not an array!");
	}

	if (json.contains("type")) {
		if (json["type"].is_string()) {
			if (json["type"] == "directional")
				Type = VehicleSirenType::Directional;
			else if (json["type"] == "non-directional")
				Type = VehicleSirenType::NonDirectional;
			else if (json["type"] == "inversed-directional")
				Type = VehicleSirenType::Inversed;
			else if (json["type"] == "rotator") {
				Type = VehicleSirenType::Rotator;

				if (json.contains("rotator")) {
					if(json["rotator"].is_object())
						Rotator = new VehicleSirenRotator(json["rotator"]);
					else
						gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", rotator property is not an object!");
				}
			}
			else
				gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", type property '" + (std::string)json["type"] + "' is not a recognized type!");
		}
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", type property is not a string!");
	}

	if (json.contains("shadow")) {
		if (json["shadow"].is_object()) {
			if (json["shadow"].contains("size")) {
				if(json["shadow"]["size"].is_number())
					Shadow.Size = json["size"];
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property size is not an acceptable number!");
			}

			if (json["shadow"].contains("type")) {
				if (json["shadow"]["type"].is_number()) {
					if (json["shadow"]["type"] == 1)
						Shadow.Type = "pointlight256";
					else if (json["shadow"]["type"] == 0)
						Shadow.Type = "taillight256";
				}
				else if(json["shadow"]["type"].is_string())
					Shadow.Type = json["shadow"]["type"];
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property type is not an acceptable number or string!");
			}

			if (json["shadow"].contains("offset")) {
				if (json["shadow"]["offset"].is_number())
					Shadow.Offset = json["shadow"]["offset"];
				else
					gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property offset is not an acceptable number!");
			}
		}
		else {
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow property is not an object!");
		}
	}

	if (json.contains("delay")) {
		if(json["delay"].is_number())
			Delay = json["delay"];
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", delay property is not an acceptable number!");
	}

	if (json.contains("inertia")) {
		if(json["inertia"].is_number())
			Inertia = json["inertia"];
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", inertia property is not an acceptable number!");
	}

	if (json.contains("ImVehFt")) {
		if(json["ImVehFt"].is_boolean() || json["ImVehFt"].is_number())
			ImVehFt = json["ImVehFt"];
		else
			gLogger->error("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", ImVehFt property is not a boolean or number!");
	}

	Validate = true;
};

VehicleSirenState::VehicleSirenState(std::string state, nlohmann::json json) {
	Name = state;

	Paintjob = -1;
	
	if (json.size() == 0) {
		gLogger->error("Failed to set up state " + state + ", could not find any keys in the manifest!\n");

		return;
	}

	for (nlohmann::json::iterator material = json.begin(); material != json.end(); ++material) {
		if (material.key() == "paintjob") {
			Paintjob = material.value();

			continue;
		}

		int materialIndex = std::stoi(material.key());

		Materials[materialIndex] = new VehicleSirenMaterial(state, materialIndex, material.value());

		if (!Materials[materialIndex]->Validate) {
			Materials.erase(materialIndex);

			gLogger->error("Failed to set up state " + state + "'s material " + material.key() + ", could not configure object from manifest!\n");

			continue;
		}
	}

	Validate = true;
};

std::map<std::string, nlohmann::json> VehicleSirenData::References;
std::map<std::string, nlohmann::json> VehicleSirenData::ReferenceColors;

VehicleSirenData::VehicleSirenData(nlohmann::json json) {
	if (json.size() == 0) {
		gLogger->error("Failed to set up states, could not find any keys in the manifest!\n");

		return;
	}

	VehicleSirenData::References.clear();
	VehicleSirenData::ReferenceColors.clear();

	nlohmann::json states = json;

	if (json.contains("states")) {
		states = json["states"];

		if (json.contains("references")) {
			nlohmann::json references = json["references"];

			for (nlohmann::json::iterator reference = references.begin(); reference != references.end(); ++reference) {
				std::string key = reference.key();

				nlohmann::json value = reference.value();

				if (key == "colors") {
					for (nlohmann::json::iterator color = value.begin(); color != value.end(); ++color)
						VehicleSirenData::ReferenceColors[color.key()] = color.value();

					continue;
				}

				VehicleSirenData::References[key] = value;
			}
		}
	}

	for (nlohmann::json::iterator stateObject = states.begin(); stateObject != states.end(); ++stateObject) {
		VehicleSirenState* state = new VehicleSirenState(stateObject.key(), stateObject.value());

		if (!state->Validate) {
			gLogger->error("Failed to set up state " + stateObject.key() + ", could not configure object from manifest!\n");

			continue;
		}

		States.push_back(state);
	}

	Validate = true;
};


void VehicleSirens::RegisterMaterial(CVehicle* vehicle, RpMaterial* material) {
	int color = material->color.red;

	if(modelData.contains(vehicle->m_nModelIndex)) {
		material->color.red = material->color.blue = material->color.green = 255;
		modelData[vehicle->m_nModelIndex]->Materials[color].push_back(new VehicleMaterial(material));
	}
};

void VehicleSirens::ParseConfig() {
	std::string path = std::string(MOD_DATA_PATH("sirens\\"));
	std::map<int, nlohmann::json> tempData;

	for (auto& p : std::filesystem::recursive_directory_iterator(path)) {
		std::string file = p.path().stem().string();
		std::string ext = p.path().extension().string();

		if (ext != ".eml" && ext != ".json")
			continue;

		gLogger->info("Reading siren config {}{}", file, ext);
		std::ifstream infile(path + file + ext);
		bool isImVehFt = false;
		try {
			int model = -1;
			if (ext == ".eml") {
				std::string line;
				if (!std::getline(infile, line)) {
					gLogger->error("Failed parsing first line");
					continue;
				}
				std::istringstream iss(line);

				if (!(iss >> model)) {
					gLogger->error("Failed parsing modelID");

					continue;
				}

				tempData[model] = {};
				tempData[model]["ImVehFt"] = {};

				while (std::getline(infile, line)) {
					if (line[0] == '#')
						continue;

					std::istringstream iss(line);

					int id, parent, type, shadow, switches, starting;

					int red, green, blue, alpha;

					float size, flash;

					if (!(iss >> id >> parent))
						continue;

					std::string tempColor;

					if (!(iss >> tempColor))
						continue;

					red = ImVehFt_ReadColor(tempColor);

					if (!(iss >> tempColor))
						continue;

					green = ImVehFt_ReadColor(tempColor);

					if (!(iss >> tempColor))
						continue;

					blue = ImVehFt_ReadColor(tempColor);

					if (!(iss >> tempColor))
						continue;

					alpha = ImVehFt_ReadColor(tempColor);

					if (!(iss >> type >> size >> shadow >> flash))
						continue;

					if (!(iss >> switches >> starting))
						continue;

					std::vector<uint64_t> pattern;

					uint64_t count = 0;

					for (int index = 0; index < switches; index++) {
						std::string string;

						if (!(iss >> string))
							continue;

						uint64_t milliseconds = std::stoi(string) - count;

						count += milliseconds;

						if (milliseconds == 0)
							continue;

						pattern.push_back(milliseconds);
					}

					if (count == 0 || count > 64553) {
						starting = 1;

						pattern.clear();
					}

					if (!tempData[model]["ImVehFt"].contains(std::to_string(256 - id)))
						tempData[model]["ImVehFt"][std::to_string(256 - id)] = {};

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("size"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["size"] = size;

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("color"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["color"] = { { "red", red }, { "green", green }, { "blue", blue }, { "alpha", alpha } };

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("state"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["state"] = starting;

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("pattern"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["pattern"] = pattern;

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("shadow"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["shadow"]["size"] = shadow;

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("inertia"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["inertia"] = flash / 100.0f;

					if (!tempData[model]["ImVehFt"][std::to_string(256 - id)].contains("type"))
						tempData[model]["ImVehFt"][std::to_string(256 - id)]["type"] = ((type == 0) ? ("directional") : ((type == 1) ? ("inversed-directional") : ((type == 4) ? ("non-directional") : ("directional"))));

					tempData[model]["ImVehFt"][std::to_string(256 - id)]["ImVehFt"] = true;
				}
				isImVehFt = true;
			} else {
				model = std::stoi(file);
				tempData[model] = nlohmann::json::parse(infile);
			}

			CurrentModel = model;
			modelData[CurrentModel] = new VehicleSirenData(tempData[model]);
			modelData[CurrentModel]->isImVehFtSiren = isImVehFt;
			if (!modelData[CurrentModel]->Validate) {
				gLogger->error("Failed to read siren configuration, cannot configure JSON manifest!\n");
				modelData.erase(CurrentModel);
			}
		}
		catch (...) {
			gLogger->error("Failed parsing {}{}", file, ext);
			continue;
		}
		infile.close();
	}
};

void VehicleSirens::Initialize() {
	ParseConfig();

	VehicleMaterials::Register([](CVehicle* vehicle, RpMaterial* material) {
		if (modelData.contains(vehicle->m_nModelIndex) && modelData[vehicle->m_nModelIndex]->isImVehFtSiren) {
			if ((std::string(material->texture->name).find("siren", 0) != 0 || std::string(material->texture->name).find("vehiclelights128", 0) != 0)
			&& (material->color.red >= 240 && material->color.green == 0 && material->color.blue == 0) ) {
				RegisterMaterial(vehicle, material);
			}
		} else {
			RegisterMaterial(vehicle, material);
		}

		return material;
	});

	VehicleMaterials::RegisterDummy([](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
		if (!modelData.contains(vehicle->m_nModelIndex))
			return;

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		if (!vehicleData.contains(index))
			vehicleData[index] = new VehicleSiren(vehicle);

		int start = -1;

		if (name.rfind("siren_", 0) == 0)
			start = 6;
		else if (name.rfind("siren", 0) == 0)
			start = 5;
		else if (name.rfind("light_em", 0) == 0)
			start = 8;

		if (start == -1)
			return;

		std::string material;

		for (int _char = start, size = name.size(); _char < size; _char++) {
			if (!isdigit(name[_char])) {
				if (_char == start)
					return;

				break;
			}

			material += name[_char];
		}

		int mat = std::stoi(material);

		if (start == 8)
			mat = 256 - mat;

		vehicleData[index]->Dummies[mat].push_back(new VehicleDummy(frame, name, parent, eDummyPos::None));
	});

	plugin::Events::vehicleCtorEvent += [](CVehicle* vehicle) {
		int model = vehicle->m_nModelIndex;

		if (!modelData.contains(model))
			return;

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		vehicleData[index] = new VehicleSiren(vehicle);
	};

	plugin::Events::vehicleDtorEvent += [](CVehicle* vehicle) {
		int model = vehicle->m_nModelIndex;

		if (!modelData.contains(model))
			return;

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		vehicleData.erase(index);
	};

	Events::processScriptsEvent += []() {
		CVehicle *vehicle = FindPlayerVehicle(-1, false);
		if (!vehicle) {
			return;
		}
		if (KeyPressed(VK_L)) {
			int model = vehicle->m_nModelIndex;

			if (!modelData.contains(model))
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (!vehicleData.contains(index))
				return;

			vehicleData[index]->Mute = !vehicleData[index]->Mute;

			if (vehicleData[index]->Mute)
				vehicle->m_nVehicleFlags.bSirenOrAlarm = false;
		}

		if (KeyPressed(VK_R)) {
			int model = vehicle->m_nModelIndex;

			if (!modelData.contains(model))
				return;

			int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

			if (!vehicleData.contains(index))
				return;

			if (!vehicleData[index]->GetSirenState())
				return;

			if (modelData[model]->States.size() == 0)
				return;

			int addition = (plugin::KeyPressed(0x10)) ? (-1) : (1);

			vehicleData[index]->State += addition;

			if (vehicleData[index]->State == modelData[model]->States.size())
				vehicleData[index]->State = 0;

			if (vehicleData[index]->State == -1)
				vehicleData[index]->State = modelData[model]->States.size() - 1;

			while (modelData[model]->States[vehicleData[index]->State]->Paintjob != -1 && modelData[model]->States[vehicleData[index]->State]->Paintjob != vehicle->GetRemapIndex()) {
				vehicleData[index]->State += (plugin::KeyPressed(0x10)) ? (-1) : (1);

				if (vehicleData[index]->State == modelData[model]->States.size()) {
					vehicleData[index]->State = 0;

					break;
				}
				else if (vehicleData[index]->State == -1) {
					vehicleData[index]->State = modelData[model]->States.size() - 1;

					break;
				}
			}
		}

		for (int number = 0; number < 10; number++) {
			if (KeyPressed(VK_0 + number)) { // 0 -> 9
				int model = vehicle->m_nModelIndex;

				if (!modelData.contains(model))
					return;

				int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

				if (!vehicleData.contains(index))
					return;

				if (!vehicleData[index]->GetSirenState())
					return;

				if (modelData[model]->States.size() == 0)
					return;

				int newState = number;

				if ((int)modelData[model]->States.size() <= newState)
					return;

				if (vehicleData[index]->State == newState)
					return;

				if (modelData[model]->States[newState]->Paintjob != -1 && modelData[model]->States[newState]->Paintjob != vehicle->GetRemapIndex())
					return;

				vehicleData[index]->State = newState;
			}
		}
	};

	VehicleMaterials::RegisterRender([](CVehicle* vehicle) {
		int model = vehicle->m_nModelIndex;
		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		if (!modelData.contains(model)) {
			return;
		}

		if (modelRotators.contains(model)) {
			for (auto& dummy: modelRotators[model]) {
				dummy->ResetAngle();
			}
			modelRotators.erase(model);
		}

		if (!vehicleData.contains(index))
			vehicleData[index] = new VehicleSiren(vehicle);

		bool sirenState = vehicleData[index]->GetSirenState();

		VehicleSirenState* state = modelData[model]->States[vehicleData[index]->GetCurrentState()];
		if (vehicleData[index]->SirenState == false && sirenState == true) {
			vehicleData[index]->SirenState = true;

			uint64_t time = Common::TimeSinceEpochMillisec();
			if (vehicleData[index]->Delay != 0) {
				vehicleData[index]->Delay = 0;
			}

			for (auto &mat: state->Materials) {
				mat.second->ColorTime = time;
				mat.second->PatternTime = time;
			}
		}
		else if (vehicleData[index]->SirenState == true && sirenState == false) {
			vehicleData[index]->SirenState = false;
		}

		if (!vehicleData[index]->GetSirenState() && !vehicleData[index]->Trailer) {
			return;
		}

		uint64_t time = Common::TimeSinceEpochMillisec();
		if (vehicleData[index]->Delay == 0) {
			vehicleData[index]->Delay = time;
		}

		for (auto& mat: state->Materials) {
			if (mat.second->Delay != 0) {
				if (time - vehicleData[index]->Delay < mat.second->Delay) {
					if (mat.second->Type == VehicleSirenType::Rotator) {
						if ((time - mat.second->Rotator->TimeElapse) > mat.second->Rotator->Time) {
							mat.second->Rotator->TimeElapse = time;

							mat.second->ResetColor(time);

							if (mat.second->Rotator->Direction == 2) {
								mat.second->Rotator->Direction = 3;
							}
							else if (mat.second->Rotator->Direction == 3) {
								mat.second->Rotator->Direction = 2;
							}
						}
					}
					continue;
				}
			}

			if (mat.second->ColorTotal != 0) {
				if ((time - mat.second->ColorTime) >= mat.second->Colors[mat.second->ColorCount].first) {
					mat.second->ColorTime = time;
					RwRGBA color = mat.second->Colors[mat.second->ColorCount].second;
					mat.second->Color = { color.red, color.green, color.blue, color.alpha };
					mat.second->ColorCount++;

					if ((size_t)mat.second->ColorCount >= mat.second->Colors.size()) {
						mat.second->ColorCount = 0;
					}
				}
			}

			if (mat.second->UpdateMaterial(time)) {
				if (mat.second->PatternCount >= (int)mat.second->Pattern.size()) {
					for (std::map<int, VehicleSirenMaterial*>::iterator materialReset = state->Materials.begin(); materialReset != state->Materials.end(); ++materialReset) {
						if (mat.second->PatternTotal == materialReset->second->PatternTotal) {
							materialReset->second->ResetMaterial(time);
						}
					}
				}
			}
			else if(mat.second->Type == VehicleSirenType::Rotator) {
				uint64_t elapsed = time - mat.second->Rotator->TimeElapse;
				if (elapsed > mat.second->Rotator->Time) {
					mat.second->Rotator->TimeElapse = time;
					mat.second->ResetColor(time);

					if (mat.second->Rotator->Direction == 2) {
						mat.second->Rotator->Direction = 3;
					}
					else if (mat.second->Rotator->Direction == 3) {
						mat.second->Rotator->Direction = 2;
					}
				}
			}
		}

		float vehAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;
		float camAngle = (TheCamera.GetHeading() * 180.0f) / 3.14f;
		CVector distance = vehicle->GetPosition() - TheCamera.GetPosition();
		eCoronaFlareType type = FLARETYPE_NONE;


		if (distance.Magnitude() > 30.0f) {
			type = FLARETYPE_HEADLIGHTS;
		}

		for (auto& mat: state->Materials) {
			if (!mat.second->State) {
				continue;
			}

			if (mat.second->Delay != 0 && time - vehicleData[index]->Delay < mat.second->Delay) {
				continue;
			}

			if (mat.second->PatternTotal != 0 && mat.second->Inertia != 0.0f) {
				float currentTime = (float)(time - mat.second->PatternTime);
				float changeTime = (((float)mat.second->Pattern[mat.second->PatternCount]) / 2.0f) * mat.second->Inertia;
				float patternTotalTime = (float)mat.second->Pattern[mat.second->PatternCount];
				mat.second->InertiaMultiplier = 1.0f;

				if (currentTime < changeTime) {
					mat.second->InertiaMultiplier = (currentTime / changeTime);
				}
				else if (currentTime > (patternTotalTime - changeTime)) {
					currentTime = patternTotalTime - currentTime;
					mat.second->InertiaMultiplier = (currentTime / changeTime);
				}
			}

			int id = 0;
			for (auto &e: vehicleData[index]->Dummies[mat.first]) {
				id++;
				EnableDummy((mat.first * 16) + id, e, vehicle, vehAngle, camAngle, mat.second, type, time);
			}

			if (mat.second->Frames != 0) {
				for (auto& e: modelData[model]->Materials[mat.first]) {
					EnableMaterial(e, mat.second, time);
				}
			}

			mat.second->Frames++;
		}
	});

	patch::ReplaceFunctionCall(0x6D8492, hkUsesSiren);

	Events::initGameEvent += [] {
		injector::MakeCALL((void*)0x6ABA60, hkRegisterCorona, true);
		injector::MakeCALL((void*)0x6BD4DD, hkRegisterCorona, true);
		injector::MakeCALL((void*)0x6BD531, hkRegisterCorona, true);
	};
};

void VehicleSirens::hkRegisterCorona(unsigned int id, CEntity* attachTo, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector const& posn, float radius, float farClip, eCoronaType coronaType, eCoronaFlareType flaretype, bool enableReflection, bool checkObstacles, int _param_not_used, float angle, bool longDistance, float nearClip, unsigned char fadeState, float fadeSpeed, bool onlyFromBelow, bool reflectionDelay) {
	CVehicle* vehicle = NULL;

	_asm {
		pushad
		mov vehicle, esi
		popad
	}

	if (vehicle && modelData.contains(vehicle->m_nModelIndex)) {
		return;
	}

	CCoronas::RegisterCorona(id, attachTo, red, green, blue, alpha, posn, radius, farClip, coronaType, flaretype, enableReflection, checkObstacles, _param_not_used, angle, longDistance, nearClip, fadeState, fadeSpeed, onlyFromBelow, reflectionDelay);
};

void VehicleSirens::EnableMaterial(VehicleMaterial* material, VehicleSirenMaterial* mat, uint64_t time) {
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->texture), *reinterpret_cast<unsigned int*>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int*>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = 1.0f + (3.0f * mat->InertiaMultiplier);

	if (mat->Diffuse.Color) {
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->color), *reinterpret_cast<unsigned int*>(&material->Material->color)));

		material->Material->color.red = mat->Color.red;
		material->Material->color.green = mat->Color.green;
		material->Material->color.blue = mat->Color.blue;

		if (mat->Diffuse.Transparent)
			material->Material->color.alpha = 255;
	}
	else if (mat->Diffuse.Transparent) {
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int*>(&material->Material->color), *reinterpret_cast<unsigned int*>(&material->Material->color)));

		material->Material->color.alpha = 255;
	}
};

void VehicleSirens::EnableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle, VehicleSirenMaterial* material, eCoronaFlareType type, uint64_t time) {
	CVector position = reinterpret_cast<CVehicleModelInfo*>(CModelInfo__ms_modelInfoPtrs[vehicle->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];

	position.x = dummy->Position.x;
	position.y = dummy->Position.y;
	position.z = dummy->Position.z;

	char alpha = material->Color.alpha;

	if (material->PatternTotal != 0 && material->Inertia != 0.0f) {
		float alphaFloat = static_cast<float>(alpha);

		alphaFloat = (alphaFloat < 0.0f) ? (alphaFloat * -1) : (alphaFloat);

		alpha = static_cast<char>(alphaFloat * material->InertiaMultiplier);
	}

	if (material->Type != VehicleSirenType::NonDirectional) {
		float dummyAngle = vehicleAngle + dummy->Angle;

		if (material->Type == VehicleSirenType::Rotator) {
			uint64_t elapsed = time - material->Rotator->TimeElapse;

			float angle = ((elapsed / ((float)material->Rotator->Time)) * material->Rotator->Radius);

			if (material->Rotator->Direction == 0)
				angle = 360.0f - angle;
			else if (material->Rotator->Direction == 2) {
				angle += material->Rotator->Offset;

				angle = 360.0f - angle;
			}
			else if (material->Rotator->Direction == 3) {
				angle += material->Rotator->Offset;
			}

			dummyAngle += angle;

			VehicleSirens::modelRotators[vehicle->m_nModelIndex].push_back(dummy);

			dummy->SetAngle(angle);

			while (dummyAngle > 360.0f)
				dummyAngle -= 360.0f;

			while (dummyAngle < 0.0f)
				dummyAngle += 360.0f;
		}
		else if(material->Type == VehicleSirenType::Inversed)
			dummyAngle -= 180.0f;

		Common::RegisterCoronaWithAngle(vehicle, position, material->Color.red, material->Color.green, material->Color.blue, alpha, 
			(reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id, cameraAngle, dummyAngle, material->Radius, material->Size);
		VehicleSirens::EnableShadow(vehicle, dummy, material, position);
	}
	else {
		Common::RegisterCorona(vehicle, position, material->Color.red, material->Color.green, material->Color.blue, alpha,
			(reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id, material->Size);
		VehicleSirens::EnableShadow(vehicle, dummy, material, position);
	}
};

void VehicleSirens::EnableShadow(CVehicle* vehicle, VehicleDummy* dummy, VehicleSirenMaterial* material, CVector position) {
	if (material->Shadow.Size == 0.0f) {
		return;
	}

	CVector center = vehicle->TransformFromObjectSpace(
		CVector(
			position.x + (material->Shadow.Offset * cos((90.0f - dummy->Angle + dummy->CurrentAngle) * 3.14f / 180.0f)),
			position.y + ((0.5f + material->Shadow.Offset) * sin((90.0f - dummy->Angle + dummy->CurrentAngle) * 3.14f / 180.0f)),
			position.z
		)
	);

	float fAngle = vehicle->GetHeading() + (((dummy->Angle + dummy->CurrentAngle) + 180.0f) * 3.14f / 180.0f);

	CVector up = CVector(-sin(fAngle), cos(fAngle), 0.0f);

	CVector right = CVector(cos(fAngle), sin(fAngle), 0.0f);

	char alpha = material->Color.alpha;

	alpha = static_cast<char>((static_cast<float>(alpha) * -1) * material->InertiaMultiplier);

	CShadows::StoreShadowToBeRendered(2, Common::GetTexture(material->Shadow.Type), &center,
		up.x, up.y,
		right.x, right.y,
		alpha, material->Color.red, material->Color.green, material->Color.blue,
		2.0f, false, 1.0f, 0, true);
}


VehicleSiren::VehicleSiren(CVehicle* _vehicle) {
	vehicle = _vehicle;

	int model = vehicle->m_nModelIndex;

	CVehicleModelInfo* modelInfo = reinterpret_cast<CVehicleModelInfo*>(CModelInfo__ms_modelInfoPtrs[model]);

	if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_HELI || modelInfo->m_nVehicleType == eVehicleType::VEHICLE_PLANE)
		this->Mute = true;

	SirenState = _vehicle->m_nVehicleFlags.bSirenOrAlarm;

	if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_TRAILER)
		Trailer = true;
};
