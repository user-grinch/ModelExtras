#include "pch.h"
#include "imvehft.h"
#include <plugin.h>

int ImVehFt_ReadColor(std::string input) {
    if (input.length() == 3)
        return std::stoi(input);

    std::istringstream stream(input);
    int color;
    stream >> std::hex >> color;
    return color;
};

std::string ConvertEMLtoJSON(std::string filename) {
	nlohmann::json tempData;
	std::string line;
	int currentModel;
	std::string emlPath = MOD_DATA_PATH("sirens/") + filename + ".eml";

	std::ifstream stream(emlPath);
	if (!std::getline(stream, line)) {
		gLogger->warn("EMLtoJSONConverter: Failed to read the first line");
		return "";
	}
	std::istringstream iss(line);

	if (!(iss >> currentModel)) {
		gLogger->warn("EMLtoJSONConverter: Failed to read the model ID");
		return "";
	}

	gLogger->info("Converting ImVehFt {}.eml -> {}.json", filename, currentModel);
	tempData[currentModel] = {};

	if (!tempData[currentModel].count("ImVehFt"))
		tempData[currentModel]["ImVehFt"] = {};
		
	try {
		while (std::getline(stream, line)) {
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

			if (!tempData[currentModel]["ImVehFt"].contains(std::to_string(256 - id)))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)] = {};

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("size"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["size"] = size;

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("color"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["color"] = { { "red", red }, { "green", green }, { "blue", blue }, { "alpha", alpha } };

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("state"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["state"] = starting;

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("pattern"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["pattern"] = pattern;

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("shadow"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["shadow"]["size"] = shadow;

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("inertia"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["inertia"] = flash / 100.0f;

			if (!tempData[currentModel]["ImVehFt"][std::to_string(256 - id)].contains("type"))
				tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["type"] = ((type == 0) ? ("directional") : ((type == 1) ? ("inversed-directional") : ((type == 4) ? ("non-directional") : ("directional"))));

			tempData[currentModel]["ImVehFt"][std::to_string(256 - id)]["ImVehFt"] = true;
		}
	} catch (...) {
		gLogger->warn("An exception occurred trying to read sirens\\{}.eml!", filename);
		return "";
	}


	// Create JSON file
	std::string jsonPath = MOD_DATA_PATH("sirens/") + std::to_string(currentModel) + ".json";
	std::ofstream file(jsonPath);
	file << std::setw(4) << tempData;
	file.close();

	stream.close();
	if (gConfig.ReadBoolean("MISC", "DeleteEML", true)) {
		std::filesystem::remove(emlPath);
	}

	return std::to_string(currentModel);
}