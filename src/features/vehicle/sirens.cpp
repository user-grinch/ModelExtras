#include "pch.h"
#include "sirens.h"

#include <CShadows.h>

int ImVehFt_ReadColor(std::string input) {
    if (input.length() == 3)
        return std::stoi(input);

    std::istringstream stream(input);

    int color;

    stream >> std::hex >> color;

    return color;
};

void PluginCoronas_Add(CEntity* attachTo, int id, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector const& posn, float size, float farClip, eCoronaType coronaType, eCoronaFlareType flaretype) {
	return CCoronas::RegisterCorona(id, attachTo, red, green, blue, alpha, posn,
		size, farClip, coronaType, flaretype, false, false, 0, 0.0f, false, 0.5f, 0, 100.0f, false, false);
};

void PluginCoronas_AddWithAngle(CEntity* attachTo, int id, float cameraAngle, float angle, float radius, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector const& posn, float size, float farClip, eCoronaType coronaType, eCoronaFlareType flaretype) {
	float differenceAngle = ((cameraAngle > angle) ? (cameraAngle - angle) : (angle - cameraAngle));

	float diameter = (radius / 2.0f);

	if (differenceAngle < diameter || differenceAngle > (360.0f - diameter))
		return;

	// if (PluginConfig::Lights->Enhancement->InertiaEnabled) {
		float alphaFloat = static_cast<float>(alpha);

		alphaFloat = (alphaFloat < 0.0f) ? (alphaFloat * -1) : (alphaFloat);

		if (differenceAngle < diameter + 15.0f) { // 15.0f
			float angle = diameter - differenceAngle;

			float multiplier = (angle / 15.0f);

			alpha = static_cast<char>(alphaFloat * multiplier);
		}
		else if (differenceAngle > (360.0f - diameter) - 15.0f) {
			float angle = 15.0f - (differenceAngle - ((360.0f - diameter) - 15.0f));

			float multiplier = angle / 15.0f;

			alpha = static_cast<char>(alphaFloat * multiplier);
		}
	// }

	return PluginCoronas_Add(attachTo, id, red, green, blue, alpha, posn, size, farClip, coronaType, flaretype);
};

std::map<std::string, RwTexture*> Textures;

RwTexture* loadTextureFromFile(const char* filename) {
    RwImage* image = RtPNGImageRead(filename);

    RwInt32 width, height, depth, flags;
    RwImageFindRasterFormat(image, 4, &width, &height, &depth, &flags);

    RwRaster* raster = RwRasterCreate(width, height, depth, flags);

    RwRasterSetFromImage(raster, image);

    RwImageDestroy(image);

    return RwTextureCreate(raster);
};

RwTexture* PluginTextures_GetTexture(std::string texture) {
	if (Textures.contains(texture) && Textures[texture])
		return Textures[texture];
	Textures[texture] = loadTextureFromFile(std::format("{}textures/{}.png", MOD_DATA_PATH(), texture).c_str());

    return Textures[texture];
};

int VehicleSirens::CurrentModel = -1;

bool VehicleSiren::GetSirenState() {
	return (Mute == false) ? (vehicle->m_nVehicleFlags.bSirenOrAlarm) : (true);
};


VehicleSirenMaterial::VehicleSirenMaterial(std::string state, int material, nlohmann::json json) {
	if (json.size() == 0) {
		gLogger->warn("Failed to set up state {}'s material {}, could not find any keys in the manifest!", state, material);

		VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + " is empty!");

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
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference array property is not an string!");
			}
		}
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference property is not an array or string!");

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
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference array property '" + (*ref) + "' is not an recognized reference!");
			}
		}
	}

	if (json.contains("size")) {
		if(json["size"].is_number())
			Size = json["size"];
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", size property is not an acceptable number!");
	}

	if (json.contains("diffuse")) {
		if(json["diffuse"].is_boolean())
			Diffuse.Color = json["diffuse"];
		else if (json["diffuse"].is_object()) {
			if (json["diffuse"].contains("color")) {
				if (json["diffuse"]["color"].is_boolean())
					Diffuse.Color = json["diffuse"]["color"];
				else
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property color is not an boolean!");
			}

			if (json["diffuse"].contains("transparent")) {
				if (json["diffuse"]["transparent"].is_boolean())
					Diffuse.Transparent = json["diffuse"]["transparent"];
				else
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property transparent is not an boolean!");
			}
		}
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property is not an boolean or object!");
	}

	if (json.contains("radius")) {
		if(json["radius"].is_number())
			Radius = json["radius"];
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", radius property is not an acceptable number!");
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
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", color property is not an object!");
	}

	if (json.contains("state")) {
		if (json["state"].is_boolean() || json["state"].is_number()) {
			bool state = (json["state"].is_boolean()) ? ((bool)json["state"]) : (((json["state"] == 0) ? (false) : (true)));

			State = state;

			StateDefault = state;
		}
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", state property is not an boolean or number!");
	}

	if (json.contains("colors")) {
		if (json["colors"].is_array()) {
			for (nlohmann::json::iterator pattern = json["colors"].begin(); pattern != json["colors"].end(); ++pattern) {
				nlohmann::json value = pattern.value();

				if (!value.is_array()) {
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property is not an array!");

					continue;
				}

				if (value.size() != 2) {
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property is not an array with a pair value of delay and color!");

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
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property does not contain an array!");
			}
		}
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors property is not an array!");
	}

	if (json.contains("pattern")) {
		if (json["pattern"].is_array()) {
			for (nlohmann::json::iterator pattern = json["pattern"].begin(); pattern != json["pattern"].end(); ++pattern) {
				nlohmann::json value = pattern.value();

				if (value.is_array()) {
					if (value.size() < 2) {
						VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern array property is an array but does not contain an iteration and pattern setting!");

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
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern array property is not an array or number!");
			}
		}
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern property is not an array!");
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
						VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", rotator property is not an object!");
				}
			}
			else
				VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", type property '" + (std::string)json["type"] + "' is not a recognized type!");
		}
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", type property is not a string!");
	}

	if (json.contains("shadow")) {
		if (json["shadow"].is_object()) {
			if (json["shadow"].contains("size")) {
				if(json["shadow"]["size"].is_number())
					Shadow.Size = json["size"];
				else
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property size is not an acceptable number!");
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
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property type is not an acceptable number or string!");
			}

			if (json["shadow"].contains("offset")) {
				if (json["shadow"]["offset"].is_number())
					Shadow.Offset = json["shadow"]["offset"];
				else
					VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property offset is not an acceptable number!");
			}
		}
		else {
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow property is not an object!");
		}
	}

	if (json.contains("delay")) {
		if(json["delay"].is_number())
			Delay = json["delay"];
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", delay property is not an acceptable number!");
	}

	if (json.contains("inertia")) {
		if(json["inertia"].is_number())
			Inertia = json["inertia"];
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", inertia property is not an acceptable number!");
	}

	if (json.contains("ImVehFt")) {
		if(json["ImVehFt"].is_boolean() || json["ImVehFt"].is_number())
			ImVehFt = json["ImVehFt"];
		else
			VehicleSirens::ShowWarning("Model " + std::to_string(VehicleSirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", ImVehFt property is not a boolean or number!");
	}

	Validate = true;
};

VehicleSirenState::VehicleSirenState(std::string state, nlohmann::json json) {
	Name = state;

	Paintjob = -1;
	
	if (json.size() == 0) {
		gLogger->warn("Failed to set up state {}, could not find any keys in the manifest!", state);

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

			gLogger->warn("Failed to set up state {}'s material {}, could not configure object from manifest!", state, material.key());

			continue;
		}
	}

	Validate = true;
};

std::map<std::string, nlohmann::json> VehicleSirenData::References;
std::map<std::string, nlohmann::json> VehicleSirenData::ReferenceColors;

VehicleSirenData::VehicleSirenData(nlohmann::json json) {
	if (json.size() == 0) {
		gLogger->info("Failed to set up states, could not find any keys in the manifest!");

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
			gLogger->warn("Failed to set up state {}, could not configure object from manifest!", stateObject.key());

			continue;
		}

		States.push_back(state);
	}

	Validate = true;
};

std::map<int, VehicleSiren*> VehicleSirens::vehicleData;
std::map<int, VehicleSirenData*> VehicleSirens::modelData;
std::map<int, std::vector<VehicleDummy*>> VehicleSirens::modelRotators;

std::map<int, nlohmann::json> jsonData;

void VehicleSirens::registerMaterial(CVehicle* vehicle, RpMaterial* material, bool ImVehFt) {
	int color = material->color.red;

	material->color.red = material->color.blue = material->color.green = 255;

	if(modelData.contains(vehicle->m_nModelIndex))
		modelData[vehicle->m_nModelIndex]->Materials[color].push_back(new VehicleMaterial(material));
};

void VehicleSirens::readSirenConfiguration() {
	std::string path {MOD_DATA_PATH("sirens/")};

	for (int model = 400; model < 612; model++) {
		if (!std::filesystem::exists(path + std::to_string(model) + ".json"))
			continue;

		gLogger->info("Reading siren configuration from sirens\\{}.json", model);

		std::ifstream stream(path + std::to_string(model) + ".json");


		if (!stream) {
			gLogger->warn("Failed to read siren configuration, cannot open stream!\n");

			continue;
		}

		try {
			nlohmann::json json = nlohmann::json::parse(stream);

			jsonData[model] = json;
		}
		catch (...) {
			gLogger->warn("An exception occurred trying to read sirens\\{}.json!", model);

			continue;
		}

		stream.close();
	}

	gLogger->info("Finished reading siren configurations from the sirens folder!");
};

void VehicleSirens::readSirenConfigurationIVF() {
	gLogger->info("Starting Improved Vehicle Functions (IVF) adaption process...");

	// TODO: go through the IVF EMLs to see if there's duplicates

	if (GetModuleHandleA("ImVehFt.asi"))
		gLogger->info("ImVehFt.asi is running in your game, make sure you don't have duplicated models!");

	std::string path { MOD_DATA_PATH("sirens/ImVehFt/") };

	for (auto& p : std::filesystem::recursive_directory_iterator(path)) {
		if (p.path().extension() != ".eml")
			continue;

		std::string file = p.path().stem().string();

		gLogger->info("Reading ImVehFt configuration from sirens\\ImVehFt\\{}.eml", file);

		std::string line;

		int currentModel;

		std::ifstream infile(path + file + ".eml");

		if (!std::getline(infile, line)) {
			gLogger->warn("Failed to read the file's first line? Something's wrong...");

			continue;
		}

		std::istringstream iss(line);

		if (!(iss >> currentModel)) {
			gLogger->warn("Failed to read the model ID from the file, something's wrong...");

			continue;
		}

		infile.close();

		if (!jsonData.count(currentModel))
			jsonData[currentModel] = {};

		if (!jsonData[currentModel].count("ImVehFt"))
			jsonData[currentModel]["ImVehFt"] = {};

		gLogger->info("Current ImVehFt model is {}, reading pattern data!", currentModel);

		try {
			infile.open(path + file + ".eml");

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

				if (!jsonData[currentModel]["ImVehFt"].contains(std::to_string(256 - id)))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)] = {};

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("size"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["size"] = size;

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("color"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["color"] = { { "red", red }, { "green", green }, { "blue", blue }, { "alpha", alpha } };

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("state"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["state"] = starting;

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("pattern"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["pattern"] = pattern;

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("shadow"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["shadow"]["size"] = shadow;

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("inertia"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["inertia"] = flash / 100.0f;

				if (!jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("type"))
					jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["type"] = ((type == 0) ? ("directional") : ((type == 1) ? ("inversed-directional") : ((type == 4) ? ("non-directional") : ("directional"))));

				jsonData[currentModel]["ImVehFt"][std::to_string(256 - id)]["ImVehFt"] = true;
			}

			infile.close();
		}
		catch (...) {
			gLogger->warn("An exception occurred trying to read sirens\\ImVehFt\\{}.eml!", file);
		
			continue;
		}
	}

	gLogger->info("Finished the ImVehFt adaption process!\n");
};

void VehicleSirens::registerSirenConfiguration() {
	for (std::map<int, nlohmann::json>::iterator _json = jsonData.begin(); _json != jsonData.end(); ++_json) {
		int model = _json->first;

		nlohmann::json json = _json->second;

		CurrentModel = model;

		modelData[model] = new VehicleSirenData(json);

		if (!modelData[model]->Validate) {
			gLogger->warn("Failed to read siren configuration, cannot configure JSON manifest!");

			modelData.erase(model);

			continue;
		}
	}
};

void VehicleSirens::RegisterEvents() {
	readSirenConfiguration();

	// if (PluginConfig::Helper->ImVehFt)
		readSirenConfigurationIVF();

	registerSirenConfiguration();

	VehicleMaterials::Register((VehicleMaterialFunction)[](CVehicle* vehicle, RpMaterial* material) {
		if (std::string(material->texture->name).find("siren", 0) != 0 || std::string(material->texture->name).find("vehiclelights128", 0) != 0 || material->color.green != 255 || material->color.blue != 255) {
			if(material->color.red < 240 || material->color.green != 0 || material->color.blue != 0)
				return material;

			registerMaterial(vehicle, material, true);

			return material;
		}

		registerMaterial(vehicle, material);

		return material;
	});

	// if (!PluginConfig::Siren->Enabled)
	// 	return;

	VehicleMaterials::RegisterDummy((VehicleDummyFunction)[](CVehicle* vehicle, RwFrame* frame, std::string name, bool parent) {
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

		for (std::vector<VehicleDummy*>::iterator dummy = vehicleData[index]->Dummies[mat].begin(); dummy != vehicleData[index]->Dummies[mat].end(); ++dummy) {
			//if ((*dummy)->Frame == frame)
			//	return;
		}

		vehicleData[index]->Dummies[mat].push_back(new VehicleDummy(frame, name, start, parent));
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

	if (KeyPressed(0x4C)) {
		CVehicle *vehicle = FindPlayerVehicle(-1, false);
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

	if (KeyPressed(0x52)) {
		CVehicle *vehicle = FindPlayerVehicle(-1, false);
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

	// if (PluginConfig::Keys->SirenStateNumbers) {
		for (int number = 0; number < 10; number++) {
			if (KeyPressed(0x30 + number)) {
				CVehicle *vehicle = FindPlayerVehicle(-1, false);
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

				int newState = number;//(key - PluginData::DigitKey);

				if ((int)modelData[model]->States.size() <= newState)
					return;

				if (vehicleData[index]->State == newState)
					return;

				if (modelData[model]->States[newState]->Paintjob != -1 && modelData[model]->States[newState]->Paintjob != vehicle->GetRemapIndex())
					return;

				vehicleData[index]->State = newState;

			}
		}
	// }

	VehicleMaterials::RegisterRender((VehicleMaterialRender)[](CVehicle* vehicle, int index) {
		int model = vehicle->m_nModelIndex;

		if (!modelData.contains(model))
			return;

		if (modelRotators.contains(model)) {
			for (std::vector<VehicleDummy*>::iterator dummy = modelRotators[model].begin(); dummy != modelRotators[model].end(); ++dummy)
				(*dummy)->ResetAngle();

			modelRotators.erase(model);
		}

		if (!vehicleData.contains(index))
			vehicleData[index] = new VehicleSiren(vehicle);

		bool sirenState = vehicleData[index]->GetSirenState();

		VehicleSirenState* state = modelData[model]->States[vehicleData[index]->GetCurrentState()];

		if (vehicleData[index]->SirenState == false && sirenState == true) {
			vehicleData[index]->SirenState = true;

			uint64_t time = CTimer::m_snTimeInMilliseconds;

			if (vehicleData[index]->Delay != 0)
				vehicleData[index]->Delay = 0;

			for (std::map<int, VehicleSirenMaterial*>::iterator material = state->Materials.begin(); material != state->Materials.end(); ++material) {
				material->second->ColorTime = time;
				material->second->PatternTime = time;
			}
		}
		else if (vehicleData[index]->SirenState == true && sirenState == false)
			vehicleData[index]->SirenState = false;

		if (!vehicleData[index]->GetSirenState() && !vehicleData[index]->Trailer)
			return;

		std::map<int, std::vector<VehicleDummy*>> dummies = vehicleData[index]->Dummies;
		std::map<int, std::vector<VehicleMaterial*>> materials = modelData[model]->Materials;

		uint64_t time = CTimer::m_snTimeInMilliseconds;

		if (vehicleData[index]->Delay == 0)
			vehicleData[index]->Delay = time;

		for (std::map<int, VehicleSirenMaterial*>::iterator material = state->Materials.begin(); material != state->Materials.end(); ++material) {
			if (material->second->Delay != 0) {
				if (time - vehicleData[index]->Delay < material->second->Delay) {
					if (material->second->Type == VehicleSirenType::Rotator) {
						if ((time - material->second->Rotator->TimeElapse) > material->second->Rotator->Time) {
							material->second->Rotator->TimeElapse = time;

							material->second->ResetColor(time);

							if (material->second->Rotator->Direction == 2)
								material->second->Rotator->Direction = 3;
							else if (material->second->Rotator->Direction == 3)
								material->second->Rotator->Direction = 2;
						}
					}

					continue;
				}
			}

			if (material->second->ColorTotal != 0) {
				if ((time - material->second->ColorTime) >= material->second->Colors[material->second->ColorCount].first) {
					material->second->ColorTime = time;

					RwRGBA color = material->second->Colors[material->second->ColorCount].second;

					material->second->Color = { color.red, color.green, color.blue, color.alpha };

					material->second->ColorCount++;

					if ((size_t)material->second->ColorCount >= material->second->Colors.size())
						material->second->ColorCount = 0;
				}
			}

			if (material->second->UpdateMaterial(time)) {
				if (material->second->PatternCount >= (int)material->second->Pattern.size()) {
					for (std::map<int, VehicleSirenMaterial*>::iterator materialReset = state->Materials.begin(); materialReset != state->Materials.end(); ++materialReset) {
						if (material->second->PatternTotal == materialReset->second->PatternTotal)
							materialReset->second->ResetMaterial(time);
					}
				}
			}
			else if(material->second->Type == VehicleSirenType::Rotator) {
				uint64_t elapsed = time - material->second->Rotator->TimeElapse;

				if (elapsed > material->second->Rotator->Time) {
					material->second->Rotator->TimeElapse = time;

					material->second->ResetColor(time);

					if (material->second->Rotator->Direction == 2)
						material->second->Rotator->Direction = 3;
					else if (material->second->Rotator->Direction == 3)
						material->second->Rotator->Direction = 2;
				}
			}
		}

		float vehicleAngle = (vehicle->GetHeading() * 180.0f) / 3.14f;

		float cameraAngle = (((CCamera*)0xB6F028)->GetHeading() * 180.0f) / 3.14f;

		eCoronaFlareType type = FLARETYPE_NONE;

		// if (PluginConfig::Siren->Flare->Enabled) {
			CVector distance = vehicle->GetPosition() - ((CCamera*)0xB6F028)->GetPosition();

			if (distance.Magnitude() > 30.0f) // PluginConfig::Siren->Flare->Distance
				type = FLARETYPE_HEADLIGHTS;
		// }

		for (std::map<int, VehicleSirenMaterial*>::iterator material = state->Materials.begin(); material != state->Materials.end(); ++material) {
			if (!material->second->State)
				continue;

			if (material->second->Delay != 0) {
				if (time - vehicleData[index]->Delay < material->second->Delay)
					continue;
			}

			// PluginConfig::Siren->InertiaEnabled 
			if (material->second->PatternTotal != 0 && material->second->Inertia != 0.0f) {
				float currentTime = (float)(time - material->second->PatternTime);

				float changeTime = (((float)material->second->Pattern[material->second->PatternCount]) / 2.0f) * material->second->Inertia;

				float patternTotalTime = (float)material->second->Pattern[material->second->PatternCount];

				material->second->InertiaMultiplier = 1.0f;

				if (currentTime < changeTime) {
					material->second->InertiaMultiplier = (currentTime / changeTime);
				}
				else if (currentTime > (patternTotalTime - changeTime)) {
					currentTime = patternTotalTime - currentTime;

					material->second->InertiaMultiplier = (currentTime / changeTime);
				}
			}

			int id = 0;

			for (std::vector<VehicleDummy*>::iterator dummy = dummies[material->first].begin(); dummy != dummies[material->first].end(); ++dummy) {
				id++;

				enableDummy((material->first * 16) + id, (*dummy), vehicle, vehicleAngle, cameraAngle, material->second, type, time);
			}

			if (material->second->Frames != 0) {
				for (std::vector<VehicleMaterial*>::iterator mat = materials[material->first].begin(); mat != materials[material->first].end(); ++mat)
					enableMaterial((*mat), material->second, time);
			}

			material->second->Frames++;
		}
	});

	// if (PluginConfig::Siren->DisableDefault) {
	// 	/*plugin::Events::initGameEvent += [] {
	// 		injector::MakeNOP((void*)0x6ABA60, 5, true);
	// 	};*/

	// 	plugin::Events::initGameEvent += [] {
	// 		injector::MakeCALL((void*)0x6aba60, VehicleSirens::registerCorona, true);
	// 	};
	// }
};

void VehicleSirens::registerCorona(unsigned int id, CEntity* attachTo, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector const& posn, float radius, float farClip, eCoronaType coronaType, eCoronaFlareType flaretype, bool enableReflection, bool checkObstacles, int _param_not_used, float angle, bool longDistance, float nearClip, unsigned char fadeState, float fadeSpeed, bool onlyFromBelow, bool reflectionDelay) {
	CVehicle* vehicle = NULL;

	_asm {
		pushad
		mov vehicle, esi
		popad
	}

	if (vehicle && modelData.contains(vehicle->m_nModelIndex))
		return;

	CCoronas::RegisterCorona(id, attachTo, red, green, blue, alpha, posn, radius, farClip, coronaType, flaretype, enableReflection, checkObstacles, _param_not_used, angle, longDistance, nearClip, fadeState, fadeSpeed, onlyFromBelow, reflectionDelay);
};

void VehicleSirens::enableMaterial(VehicleMaterial* material, VehicleSirenMaterial* mat, uint64_t time) {
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

void VehicleSirens::enableDummy(int id, VehicleDummy* dummy, CVehicle* vehicle, float vehicleAngle, float cameraAngle, VehicleSirenMaterial* material, eCoronaFlareType type, uint64_t time) {
	CVector position = reinterpret_cast<CVehicleModelInfo*>(CModelInfo::ms_modelInfoPtrs[vehicle->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];

	position.x = dummy->Position.x;
	position.y = dummy->Position.y;
	position.z = dummy->Position.z;

	char alpha = material->Color.alpha;

	// if (PluginConfig::Siren->InertiaEnabled && material->PatternTotal != 0 && material->Inertia != 0.0f) {
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

		PluginCoronas_AddWithAngle(vehicle, (reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id, cameraAngle, dummyAngle, material->Radius,
			material->Color.red, material->Color.green, material->Color.blue, alpha,
			position, material->Size, 300.0f, eCoronaType::CORONATYPE_HEADLIGHT, type);

		VehicleSirens::enableShadow(vehicle, dummy, material, position);
	}
	else {
		PluginCoronas_Add(vehicle, (reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id,
			material->Color.red, material->Color.green, material->Color.blue, alpha,
			position, material->Size, 300.0f, eCoronaType::CORONATYPE_HEADLIGHT, type);

		VehicleSirens::enableShadow(vehicle, dummy, material, position);
	}

	// (CPools::ms_pVehiclePool->GetIndex(vehicle) * 255) + 255 + id

	//CCoronas::RegisterCorona((CPools::ms_pVehiclePool->GetIndex(vehicle) * 255) + 255 + id, vehicle, material->Color.red, material->Color.green, material->Color.blue, alpha, position,
	//	material->Size, PluginConfig::Siren->StreamDistance, eCoronaType::CORONATYPE_HEADLIGHT, type, false, false, 0, 0.0f, false, 0.5f, 0, 50.0f, false, true);
};

void VehicleSirens::enableShadow(CVehicle* vehicle, VehicleDummy* dummy, VehicleSirenMaterial* material, CVector position) {
	// if (PluginConfig::Siren->Shadow->Enabled == false)
	// 	return;

	if (material->Shadow.Size == 0.0f) {
		// if(!PluginConfig::Siren->Shadow->ForceEnabled)
		// 	return;

		material->Shadow.Size = material->Size * 3.0f;
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

	// if (PluginConfig::Siren->InertiaEnabled)
		alpha = static_cast<char>((static_cast<float>(alpha) * -1) * material->InertiaMultiplier);

	// CShadows::StoreShadowToBeRendered(2, PluginTextures_GetTexture("siren\\" + material->Shadow.Type), &center,
	// 	up.x, up.y,
	// 	right.x, right.y,
	// 	alpha, material->Color.red, material->Color.green, material->Color.blue,
	// 	2.0f, false, 1.0f, 0, true);
};

VehicleSiren::VehicleSiren(CVehicle* _vehicle) {
	vehicle = _vehicle;

	int model = vehicle->m_nModelIndex;

	CVehicleModelInfo* modelInfo = reinterpret_cast<CVehicleModelInfo*>(CModelInfo::ms_modelInfoPtrs[model]);

	if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_HELI || modelInfo->m_nVehicleType == eVehicleType::VEHICLE_PLANE)
		this->Mute = true;

	SirenState = _vehicle->m_nVehicleFlags.bSirenOrAlarm;

	if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_TRAILER)
		Trailer = true;
};