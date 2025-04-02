#include "pch.h"
#include "sirens.h"
#include <common.h>
#include "avs/common.h"
#include <CShadows.h>
#include <rwcore.h>
#include <rpworld.h>
#include "defines.h"
#include "lights.h"
#include "../audiomgr.h"
#include <CPointLights.h>

bool VehicleSiren::GetSirenState()
{
	return (Mute == false) ? (vehicle->m_nVehicleFlags.bSirenOrAlarm) : (true);
};

bool IsEngineOff(CVehicle *pVeh);

char __fastcall Sirens::hkUsesSiren(CVehicle *ptr)
{
	if (Sirens::modelData.contains(ptr->m_nModelIndex))
	{
		ptr->m_vehicleAudio.m_bModelWithSiren = true;
		return true;
	}
	return ptr->IsLawEnforcementVehicle();
}

static CVehicle *pCurrentVeh = nullptr;
void __fastcall hkVehiclePreRender(CVehicle *ptr)
{
	pCurrentVeh = ptr;
	plugin::CallMethod<0x6D6480>(ptr);
}

void __cdecl Sirens::hkAddPointLights(uint8_t type, CVector point, CVector dir, float range, float red, float green, float blue, uint8_t fogEffect, bool bCastsShadowFromPlayerCarAndPed, CEntity *castingEntity)
{
	if (!pCurrentVeh || !Sirens::modelData.contains(pCurrentVeh->m_nModelIndex))
	{
		CPointLights::AddLight(type, point, dir, range, red, green, blue, fogEffect, bCastsShadowFromPlayerCarAndPed, castingEntity);
	}
}

VehicleSirenMaterial::VehicleSirenMaterial(std::string state, int material, nlohmann::json json)
{
	if (json.size() == 0)
	{
		gLogger->error("Model {} siren configuration exception!", Sirens::CurrentModel);
		return;
	}

	if (json.contains("reference"))
	{
		std::vector<std::string> references;

		if (json["reference"].is_string())
			references.push_back(json["reference"]);
		else if (json["reference"].is_array())
		{
			for (nlohmann::json::iterator reference = json["reference"].begin(); reference != json["reference"].end(); ++reference)
			{
				if (reference.value().is_string())
					references.push_back(reference.value());
				else
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference array property is not an string!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference property is not an array or string!");

		if (references.size() != 0)
		{
			for (std::vector<std::string>::iterator ref = references.begin(); ref != references.end(); ++ref)
			{
				if (VehicleSirenData::References.contains((*ref)))
				{
					for (nlohmann::json::iterator reference = VehicleSirenData::References[(*ref)].begin(); reference != VehicleSirenData::References[(*ref)].end(); ++reference)
					{
						if (json.contains(reference.key()))
						{
							if (json[reference.key()].is_object())
							{
								nlohmann::json object = reference.value();

								for (nlohmann::json::iterator objectReference = object.begin(); objectReference != object.end(); ++objectReference)
								{
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
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", reference array property '" + (*ref) + "' is not an recognized reference!");
			}
		}
	}

	if (json.contains("size"))
	{
		if (json["size"].is_number())
			Size = json["size"];
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", size property is not an acceptable number!");
	}

	if (json.contains("diffuse"))
	{
		if (json["diffuse"].is_boolean())
			Diffuse.Color = json["diffuse"];
		else if (json["diffuse"].is_object())
		{
			if (json["diffuse"].contains("color"))
			{
				if (json["diffuse"]["color"].is_boolean())
					Diffuse.Color = json["diffuse"]["color"];
				else
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property color is not an boolean!");
			}

			if (json["diffuse"].contains("transparent"))
			{
				if (json["diffuse"]["transparent"].is_boolean())
					Diffuse.Transparent = json["diffuse"]["transparent"];
				else
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property transparent is not an boolean!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", diffuse property is not an boolean or object!");
	}

	if (json.contains("radius"))
	{
		if (json["radius"].is_number())
			Radius = json["radius"];
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", radius property is not an acceptable number!");
	}

	if (json.contains("color"))
	{
		if (json["color"].is_string() && VehicleSirenData::ReferenceColors.contains(json["color"]))
			json["color"] = VehicleSirenData::ReferenceColors[json["color"]];

		if (json["color"].is_object())
		{
			if (json["color"].contains("red"))
				Color.red = json["color"]["red"];

			if (json["color"].contains("green"))
				Color.green = json["color"]["green"];

			if (json["color"].contains("blue"))
				Color.blue = json["color"]["blue"];

			if (json["color"].contains("alpha"))
				Color.alpha = json["color"]["alpha"];

			DefaultColor = {Color.red, Color.green, Color.blue, Color.alpha};
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", color property is not an object!");
	}

	if (json.contains("state"))
	{
		if (json["state"].is_boolean() || json["state"].is_number())
		{
			bool state = (json["state"].is_boolean()) ? ((bool)json["state"]) : (((json["state"] == 0) ? (false) : (true)));

			State = state;

			StateDefault = state;
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", state property is not an boolean or number!");
	}

	if (json.contains("colors"))
	{
		if (json["colors"].is_array())
		{
			for (nlohmann::json::iterator pattern = json["colors"].begin(); pattern != json["colors"].end(); ++pattern)
			{
				nlohmann::json value = pattern.value();

				if (!value.is_array())
				{
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property is not an array!");

					continue;
				}

				if (value.size() != 2)
				{
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property is not an array with a pair value of delay and color!");

					continue;
				}

				uint64_t time = value[0];

				RwRGBA color;

				if (value[1].is_string() && VehicleSirenData::ReferenceColors.contains(value[1]))
					value[1] = VehicleSirenData::ReferenceColors[value[1]];

				if (value[1].is_object())
				{
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
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors array property does not contain an array!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", colors property is not an array!");
	}

	if (json.contains("pattern"))
	{
		if (json["pattern"].is_array())
		{
			for (nlohmann::json::iterator pattern = json["pattern"].begin(); pattern != json["pattern"].end(); ++pattern)
			{
				nlohmann::json value = pattern.value();

				if (value.is_array())
				{
					if (value.size() < 2)
					{
						gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern array property is an array but does not contain an iteration and pattern setting!");

						continue;
					}

					for (int count = 0; count < value[0]; count++)
					{
						for (nlohmann::json::iterator vector = ++value.begin(); vector != value.end(); ++vector)
						{
							int time = vector.value();

							Pattern.push_back(time);

							PatternTotal += time;
						}
					}

					continue;
				}
				else if (value.is_number())
				{
					int time = pattern.value();

					Pattern.push_back(time);

					PatternTotal += time;
				}
				else
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern array property is not an array or number!");
			}
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", pattern property is not an array!");
	}

	if (json.contains("type"))
	{
		if (json["type"].is_string())
		{
			if (json["type"] == "directional")
				Type = LightType::Directional;
			else if (json["type"] == "non-directional")
				Type = LightType::NonDirectional;
			else if (json["type"] == "inversed-directional")
				Type = LightType::Inversed;
			else if (json["type"] == "rotator")
			{
				Type = LightType::Rotator;

				if (json.contains("rotator"))
				{
					if (json["rotator"].is_object())
						Rotator = new VehicleSirenRotator(json["rotator"]);
					else
						gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", rotator property is not an object!");
				}
			}
			else
				gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", type property '" + (std::string)json["type"] + "' is not a recognized type!");
		}
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", type property is not a string!");
	}

	if (json.contains("shadow"))
	{
		if (json["shadow"].is_object())
		{
			if (json["shadow"].contains("size"))
			{
				if (json["shadow"]["size"].is_number())
				{
					Shadow.Size = json["size"];
				}
				else
				{
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " has wrong shadow size. Using default value!");
				}
			}

			if (json["shadow"].contains("type"))
			{
				if (json["shadow"]["type"].is_number())
				{
					if (json["shadow"]["type"] == 1)
						Shadow.Type = "pointlight";
					else if (json["shadow"]["type"] == 0)
						Shadow.Type = DEFAULT_SIREN_SHADOW;
					else if (json["shadow"]["type"] == 10)
						Shadow.Type = "narrow";
					// TODO: Implement others
					// No docs for them
				}
				else if (json["shadow"]["type"].is_string())
				{
					Shadow.Type = json["shadow"]["type"];
				}
				else
				{
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property type is not an acceptable number or string!");
				}
			}

			if (json["shadow"].contains("offset"))
			{
				if (json["shadow"]["offset"].is_number())
					Shadow.Offset = json["shadow"]["offset"];
				else
					gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow object property offset is not an acceptable number!");
			}

			// ModelExtras
			if (Shadow.Type == "0" || Shadow.Type == DEFAULT_SIREN_SHADOW)
			{
				Shadow.Size *= 5.0f;
			}
			else
			{
				Shadow.Size *= 2.0f;
			}
		}
		else
		{
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", shadow property is not an object!");
		}
	}

	if (json.contains("delay"))
	{
		if (json["delay"].is_number())
			Delay = json["delay"];
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", delay property is not an acceptable number!");
	}

	if (json.contains("inertia"))
	{
		if (json["inertia"].is_number())
			Inertia = json["inertia"];
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", inertia property is not an acceptable number!");
	}

	if (json.contains("ImVehFt"))
	{
		if (json["ImVehFt"].is_boolean() || json["ImVehFt"].is_number())
			ImVehFt = json["ImVehFt"];
		else
			gLogger->error("Model " + std::to_string(Sirens::CurrentModel) + " siren configuration exception!\n \nState '" + state + "' material " + std::to_string(material) + ", ImVehFt property is not a boolean or number!");
	}

	Validate = true;
};

VehicleSirenState::VehicleSirenState(std::string state, nlohmann::json json)
{
	Name = state;

	Paintjob = -1;

	if (json.size() == 0)
	{
		gLogger->error("Failed to set up state " + state + ", could not find any keys in the manifest!\n");

		return;
	}

	for (nlohmann::json::iterator material = json.begin(); material != json.end(); ++material)
	{
		if (material.key() == "paintjob")
		{
			Paintjob = material.value();

			continue;
		}

		int materialIndex = std::stoi(material.key());

		Materials[materialIndex] = new VehicleSirenMaterial(state, materialIndex, material.value());

		if (!Materials[materialIndex]->Validate)
		{
			Materials.erase(materialIndex);

			gLogger->error("Failed to set up state " + state + "'s material " + material.key() + ", could not configure object from manifest!\n");

			continue;
		}
	}

	Validate = true;
};

VehicleSirenData::VehicleSirenData(nlohmann::json json)
{
	if (json.size() == 0)
	{
		gLogger->error("Failed to set up states, could not find any keys in the manifest!\n");
		return;
	}

	VehicleSirenData::References.clear();
	VehicleSirenData::ReferenceColors.clear();

	nlohmann::json states = json;

	if (json.contains("states"))
	{
		states = json["states"];

		if (json.contains("references"))
		{
			nlohmann::json references = json["references"];

			for (nlohmann::json::iterator reference = references.begin(); reference != references.end(); ++reference)
			{
				std::string key = reference.key();

				nlohmann::json value = reference.value();

				if (key == "colors")
				{
					for (nlohmann::json::iterator color = value.begin(); color != value.end(); ++color)
					{
						VehicleSirenData::ReferenceColors[color.key()] = color.value();
					}

					continue;
				}

				VehicleSirenData::References[key] = value;
			}
		}
	}

	for (nlohmann::json::iterator stateObject = states.begin(); stateObject != states.end(); ++stateObject)
	{
		VehicleSirenState *state = new VehicleSirenState(stateObject.key(), stateObject.value());

		if (!state->Validate)
		{
			gLogger->error("Failed to set up state " + stateObject.key() + ", could not configure object from manifest!\n");

			continue;
		}

		States.push_back(state);
	}

	Validate = true;
};

void Sirens::RegisterMaterial(CVehicle *vehicle, RpMaterial *material)
{

	if (modelData.contains(vehicle->m_nModelIndex))
	{
		// Don't move this out
		int id = material->color.red;
		if (modelData[vehicle->m_nModelIndex]->isImVehFtSiren)
		{
			id = 256 - id;
		}
		material->color.red = material->color.blue = material->color.green = 255;
		modelData[vehicle->m_nModelIndex]->Materials[id].push_back(new VehicleMaterial(material));
	}
};

extern int ImVehFt_EmlToJson(const std::string &emlPath);
extern bool is_number(const std::string &s);

void Sirens::ParseConfig()
{
	std::string path = std::string(MOD_DATA_PATH("data/sirens/"));
	std::map<int, nlohmann::json> tempData;

	for (auto &p : std::filesystem::directory_iterator(path))
	{
		std::string file = p.path().stem().string();
		std::string ext = p.path().extension().string();

		if (ext != ".eml" && ext != ".json")
			continue;

		if (ext == ".eml")
		{
			std::string emlFile = p.path().string();
			int model = ImVehFt_EmlToJson(emlFile);
			if (model == -1)
			{
				continue;
			}
			file = std::to_string(model);
			ext = ".json";
		}

		LOG_VERBOSE("Reading sirens/{}{}", file, ext);
		std::ifstream infile(path + file + ext);
		bool isImVehFt = (ext == ".json");

		try
		{
			int model = 0;
			if (is_number(file))
			{
				model = std::stoi(file);
			}
			else
			{
				if (!CModelInfo::GetModelInfo((char *)file.c_str(), &model))
				{
					continue; // invalid skip it
				}
			}

			if (model == 0)
			{
				continue; // skip
			}

			nlohmann::json _json = nlohmann::json::parse(infile);
			CurrentModel = model;

			modelData[CurrentModel] = new VehicleSirenData(_json);
			modelData[CurrentModel]->isImVehFtSiren = _json.contains("ImVehFt") && _json["ImVehFt"];

			if (!modelData[CurrentModel]->Validate)
			{
				gLogger->error("Failed to read siren configuration, cannot configure JSON manifest!");
				modelData.erase(CurrentModel);
			}
		}
		catch (...)
		{
			gLogger->error("Failed parsing {}{}", file, ext);
			continue;
		}
		infile.close();
	}
}

void Sirens::Initialize()
{
	Events::initGameEvent += []()
	{
		ParseConfig();
	};

	VehicleMaterials::Register([](CVehicle *vehicle, RpMaterial *material, CRGBA col)
							   {
		if (modelData.contains(vehicle->m_nModelIndex)
			&& (col.b == 255 || col.g == 255 || modelData[vehicle->m_nModelIndex]->isImVehFtSiren)) {
			if (modelData[vehicle->m_nModelIndex]->isImVehFtSiren) {
				if ((std::string(material->texture->name).find("siren", 0) != 0 || std::string(material->texture->name).find("vehiclelights128", 0) != 0)
				&& (col.r >= 240 && col.g == 0 && col.b == 0)) {
					RegisterMaterial(vehicle, material);
				}
			}
			else if (col.r > 0 && col.g == 255 && col.b == 255) {
				RegisterMaterial(vehicle, material);
			}
		}
		return material; });

	VehicleMaterials::RegisterDummy([](CVehicle *vehicle, RwFrame *frame, std::string name, bool parent)
									{
		if (!modelData.contains(vehicle->m_nModelIndex))
			return;

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		if (!vehicleData.contains(index))
			vehicleData[index] = new VehicleSiren(vehicle);

		std::regex pattern(R"(^(siren_|siren|light_em)(\d+))");
		std::smatch match;

		if (std::regex_search(name, match, pattern)) {
			int id = std::stoi(match[2]);
			vehicleData[index]->Dummies[id].push_back(new VehicleDummy(vehicle, frame, name, parent, eDummyPos::None));
		} });

	plugin::Events::vehicleCtorEvent += [](CVehicle *vehicle)
	{
		int model = vehicle->m_nModelIndex;

		if (!modelData.contains(model))
			return;

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		vehicleData[index] = new VehicleSiren(vehicle);
	};

	plugin::Events::vehicleDtorEvent += [](CVehicle *vehicle)
	{
		int model = vehicle->m_nModelIndex;

		if (!modelData.contains(model))
			return;

		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		vehicleData.erase(index);
	};

	Events::processScriptsEvent += []()
	{
		CVehicle *vehicle = FindPlayerVehicle(-1, false);
		if (!vehicle)
		{
			return;
		}

		static size_t prev = 0;
		size_t now = CTimer::m_snTimeInMilliseconds;

		if (now - prev > 300.0f)
		{
			if (KeyPressed(VK_L))
			{
				int model = vehicle->m_nModelIndex;

				if (!modelData.contains(model))
					return;

				int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

				if (!vehicleData.contains(index))
					return;

				vehicleData[index]->Mute = !vehicleData[index]->Mute;

				if (vehicleData[index]->Mute)
					vehicle->m_nVehicleFlags.bSirenOrAlarm = false;

				AudioMgr::PlayClickSound();
			}

			if (KeyPressed(VK_R))
			{
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

				while (modelData[model]->States[vehicleData[index]->State]->Paintjob != -1 && modelData[model]->States[vehicleData[index]->State]->Paintjob != vehicle->GetRemapIndex())
				{
					vehicleData[index]->State += (plugin::KeyPressed(0x10)) ? (-1) : (1);

					if (vehicleData[index]->State == modelData[model]->States.size())
					{
						vehicleData[index]->State = 0;

						break;
					}
					else if (vehicleData[index]->State == -1)
					{
						vehicleData[index]->State = modelData[model]->States.size() - 1;

						break;
					}
				}
			}
			prev = now;
		}
		for (int number = 0; number < 9; number++)
		{
			if (KeyPressed(VK_1 + number))
			{ // 1 -> 9
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

	VehicleMaterials::RegisterRender([](CVehicle *vehicle)
									 {
		int model = vehicle->m_nModelIndex;
		int index = CPools::ms_pVehiclePool->GetIndex(vehicle);

		if (!modelData.contains(model)) {
			return;
		}

		if (IsEngineOff(vehicle)) {
			vehicle->m_nVehicleFlags.bSirenOrAlarm = false;
			return;
		}

		if (modelRotators.contains(model)) {
			for (auto& dummy : modelRotators[model]) {
				dummy->ResetAngle();
			}
			modelRotators.erase(model);
		}

		if (!vehicleData.contains(index))
			vehicleData[index] = new VehicleSiren(vehicle);

		bool sirenState = vehicleData[index]->GetSirenState();
		uint64_t time = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		VehicleSirenState* state = modelData[model]->States[vehicleData[index]->GetCurrentState()];
		if (vehicleData[index]->SirenState == false && sirenState == true) {
			vehicleData[index]->SirenState = true;

			if (vehicleData[index]->Delay != 0) {
				vehicleData[index]->Delay = 0;
			}

			for (auto& mat : state->Materials) {
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

		if (vehicleData[index]->Delay == 0) {
			vehicleData[index]->Delay = time;
		}

		for (auto& mat : state->Materials) {
			if (mat.second->Delay != 0) {
				if (time - vehicleData[index]->Delay < mat.second->Delay) {
					if (mat.second->Type == LightType::Rotator) {
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
			else if (mat.second->Type == LightType::Rotator) {
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

		CVector distance = vehicle->GetPosition() - TheCamera.GetPosition();
		eCoronaFlareType type = FLARETYPE_NONE;


		if (distance.Magnitude() > 30.0f) {
			type = FLARETYPE_HEADLIGHTS;
		}

		for (auto& mat : state->Materials) {
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
			for (auto& e : vehicleData[index]->Dummies[mat.first]) {
				id++;
				EnableDummy((mat.first * 16) + id, e, vehicle, mat.second, type, time);
			}

			if (mat.second->Frames != 0) {
				for (auto& e : modelData[model]->Materials[mat.first]) {
					EnableMaterial(e, mat.second, time);
				}
			}

			mat.second->Frames++;
		} });

	patch::ReplaceFunctionCall(0x6D8492, hkUsesSiren);
	patch::ReplaceFunctionCall(0x6AB80F, hkAddPointLights);
	patch::ReplaceFunctionCall(0x6AAB71, hkVehiclePreRender);

	Events::initGameEvent += []
	{
		injector::MakeCALL((void *)0x6ABA60, hkRegisterCorona, true);
		injector::MakeCALL((void *)0x6BD4DD, hkRegisterCorona, true);
		injector::MakeCALL((void *)0x6BD531, hkRegisterCorona, true);
	};
};

void Sirens::hkRegisterCorona(unsigned int id, CEntity *attachTo, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, CVector const &posn, float radius, float farClip, eCoronaType coronaType, eCoronaFlareType flaretype, bool enableReflection, bool checkObstacles, int _param_not_used, float angle, bool longDistance, float nearClip, unsigned char fadeState, float fadeSpeed, bool onlyFromBelow, bool reflectionDelay)
{
	CVehicle *vehicle = NULL;

	_asm {
		pushad
		mov vehicle, esi
		popad
	}

	if (vehicle && modelData.contains(vehicle->m_nModelIndex))
	{
		return;
	}

	CCoronas::RegisterCorona(id, attachTo, red, green, blue, alpha, posn, radius, farClip, coronaType, flaretype, enableReflection, checkObstacles, _param_not_used, angle, longDistance, nearClip, fadeState, fadeSpeed, onlyFromBelow, reflectionDelay);
};

void Sirens::EnableMaterial(VehicleMaterial *material, VehicleSirenMaterial *mat, uint64_t time)
{
	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int *>(&material->Material->texture), *reinterpret_cast<unsigned int *>(&material->Material->texture)));

	material->Material->texture = material->TextureActive;

	VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int *>(&material->Material->surfaceProps.ambient), *reinterpret_cast<unsigned int *>(&material->Material->surfaceProps.ambient)));

	material->Material->surfaceProps.ambient = AMBIENT_ON_VAL;

	if (mat->Diffuse.Color)
	{
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int *>(&material->Material->color), *reinterpret_cast<unsigned int *>(&material->Material->color)));

		material->Material->color.red = mat->Color.red;
		material->Material->color.green = mat->Color.green;
		material->Material->color.blue = mat->Color.blue;

		if (mat->Diffuse.Transparent)
			material->Material->color.alpha = 255;
	}
	else if (mat->Diffuse.Transparent)
	{
		VehicleMaterials::StoreMaterial(std::make_pair(reinterpret_cast<unsigned int *>(&material->Material->color), *reinterpret_cast<unsigned int *>(&material->Material->color)));

		material->Material->color.alpha = 255;
	}
};

void Sirens::EnableDummy(int id, VehicleDummy *dummy, CVehicle *vehicle, VehicleSirenMaterial *material, eCoronaFlareType type, uint64_t time)
{
	CVector position = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[vehicle->m_nModelIndex])->m_pVehicleStruct->m_avDummyPos[0];
	std::string name = GetFrameNodeName(dummy->Frame);
	position.x = dummy->Position.x;
	position.y = dummy->Position.y;
	position.z = dummy->Position.z;
	dummy->Update(vehicle);
	unsigned char alpha = material->Color.alpha;

	if (material->PatternTotal != 0 && material->Inertia != 0.0f)
	{
		float alphaFloat = static_cast<float>(alpha);

		alphaFloat = (alphaFloat < 0.0f) ? (alphaFloat * -1) : (alphaFloat);

		alpha = static_cast<char>(alphaFloat * material->InertiaMultiplier);
	}

	if (material->Type != LightType::NonDirectional)
	{
		float dummyAngle = dummy->Angle;

		if (material->Type == LightType::Rotator)
		{
			uint64_t elapsed = time - material->Rotator->TimeElapse;

			float angle = ((elapsed / ((float)material->Rotator->Time)) * material->Rotator->Radius);

			if (material->Rotator->Direction == 0)
				angle = 360.0f - angle;
			else if (material->Rotator->Direction == 2)
			{
				angle += material->Rotator->Offset;

				angle = 360.0f - angle;
			}
			else if (material->Rotator->Direction == 3)
			{
				angle += material->Rotator->Offset;
			}

			dummyAngle += angle;

			Sirens::modelRotators[vehicle->m_nModelIndex].push_back(dummy);

			dummy->SetAngle(angle);

			while (dummyAngle > 360.0f)
				dummyAngle -= 360.0f;

			while (dummyAngle < 0.0f)
				dummyAngle += 360.0f;
		}
		else if (material->Type == LightType::Inversed)
		{
			dummyAngle -= 180.0f;
		}

		// Fix, probably gonna fk me later again
		if (modelData[vehicle->m_nModelIndex]->isImVehFtSiren)
		{
			dummyAngle -= 180.0f;
		}

		Common::RegisterCoronaWithAngle(vehicle, (reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id, position,
										material->Color.red, material->Color.green, material->Color.blue, material->Color.alpha,
										dummyAngle, material->Radius, material->Size);
	}
	else
	{
		Common::RegisterCorona(vehicle, (reinterpret_cast<unsigned int>(vehicle) * 255) + 255 + id, position, material->Color.red,
							   material->Color.green, material->Color.blue, material->Color.alpha, material->Size);
	}

	if (!modelData[vehicle->m_nModelIndex]->isImVehFtSiren)
	{
		CVector pos = position;
		pos.x += (pos.x > 0 ? 1.2f : -1.2f); // FIX ME!!!
		unsigned char alpha = static_cast<char>((static_cast<float>(material->Color.alpha) * -1) * material->InertiaMultiplier);

		Common::RegisterShadow(vehicle, pos, *(CRGBA *)&material->Color, dummy->Angle, dummy->CurrentAngle, material->Shadow.Type, {material->Shadow.Size, material->Shadow.Size}, {material->Shadow.Offset, material->Shadow.Offset}, nullptr);
	}
};

VehicleSiren::VehicleSiren(CVehicle *_vehicle)
{
	vehicle = _vehicle;

	int model = vehicle->m_nModelIndex;

	CVehicleModelInfo *modelInfo = reinterpret_cast<CVehicleModelInfo *>(CModelInfo__ms_modelInfoPtrs[model]);

	if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_HELI || modelInfo->m_nVehicleType == eVehicleType::VEHICLE_PLANE)
		this->Mute = true;

	SirenState = _vehicle->m_nVehicleFlags.bSirenOrAlarm;

	if (modelInfo->m_nVehicleType == eVehicleType::VEHICLE_TRAILER)
		Trailer = true;
};
