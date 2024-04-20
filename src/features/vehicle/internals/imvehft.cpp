#include "pch.h"
// #include "imvehft.h"
// #include <plugin.h>

// int ImVehFt_ReadColor(std::string input) {
//     if (input.length() == 3)
//         return std::stoi(input);

//     std::istringstream stream(input);
//     int color;
//     stream >> std::hex >> color;
//     return color;
// };

// std::string ConvertEMLtoJSON(std::string filename) {
// 	nlohmann::json tempData;
// 	std::string line;
// 	int model;
// 	std::string emlPath = MOD_DATA_PATH("sirens/") + filename + ".eml";

// 	std::ifstream stream(emlPath);
// 	if (!std::getline(stream, line)) {
// 		gLogger->warn("EMLtoJSONConverter: Failed to read the first line");
// 		return "";
// 	}
// 	std::istringstream iss(line);

// 	if (!(iss >> model)) {
// 		gLogger->warn("EMLtoJSONConverter: Failed to read the model ID");
// 		return "";
// 	}

// 	gLogger->info("Converting ImVehFt {}.eml -> {}.json", filename, model);
// 	tempData["states"] = {};
// 	try {
// 		while (std::getline(stream, line)) {
// 			if (line[0] == '#')
// 				continue;

// 			std::istringstream iss(line);
// 			int id, parent, type, shadow, switches, starting, red, green, blue, alpha;
// 			float size, flash;

// 			if (!(iss >> id >> parent))
// 				continue;

// 			std::string tempColor;

// 			if (!(iss >> tempColor)) continue;
// 			red = ImVehFt_ReadColor(tempColor);

// 			if (!(iss >> tempColor)) continue;
// 			green = ImVehFt_ReadColor(tempColor);

// 			if (!(iss >> tempColor)) continue;
// 			blue = ImVehFt_ReadColor(tempColor);

// 			if (!(iss >> tempColor)) continue;
// 			alpha = ImVehFt_ReadColor(tempColor);

// 			if (!(iss >> type >> size >> shadow >> flash >> switches >> starting))
// 				continue;

// 			std::vector<uint64_t> pattern;

// 			uint64_t count = 0;

// 			for (int index = 0; index < switches; index++) {
// 				std::string string;

// 				if (!(iss >> string))
// 					continue;

// 				uint64_t milliseconds = std::stoi(string) - count;

// 				count += milliseconds;

// 				if (milliseconds == 0)
// 					continue;

// 				pattern.push_back(milliseconds);
// 			}

// 			if (count == 0 || count > 64553) {
// 				starting = 1;
// 				pattern.clear();
// 			}

// 			std::string sid = std::to_string(id);
// 			if (!tempData["states"].contains(sid))
// 				tempData["states"][sid] = {};

// 			if (!tempData["states"][sid].contains("size"))
// 				tempData["states"][sid]["size"] = size;

// 			if (!tempData["states"][sid].contains("color"))
// 				tempData["states"][sid]["color"] = { { "red", red }, { "green", green }, { "blue", blue }, { "alpha", alpha } };

// 			if (!tempData["states"][sid].contains("state"))
// 				tempData["states"][sid]["state"] = starting;

// 			if (!tempData["states"][sid].contains("pattern"))
// 				tempData["states"][sid]["pattern"] = pattern;

// 			if (!tempData["states"][sid].contains("shadow"))
// 				tempData["states"][sid]["shadow"]["size"] = shadow;

// 			if (!tempData["states"][sid].contains("inertia"))
// 				tempData["states"][sid]["inertia"] = flash / 100.0f;

// 			if (!tempData["states"][sid].contains("type"))
// 				tempData["states"][sid]["type"] = ((type == 0) ? ("directional") : ((type == 1) ? ("inversed-directional") : ((type == 4) ? ("non-directional") : ("directional"))));

// 			// tempData["ImVehFt"][sid]["ImVehFt"] = true;
// 		}
// 	} catch (...) {
// 		gLogger->warn("An exception occurred trying to read sirens\\{}.eml!", filename);
// 		return "";
// 	}


// 	// Create JSON file
// 	std::string jsonPath = MOD_DATA_PATH("sirens/") + std::to_string(model) + ".json";
// 	std::ofstream file(jsonPath);
// 	file << std::setw(4) << tempData;
// 	file.close();

// 	stream.close();
// 	if (gConfig.ReadBoolean("MISC", "DeleteEML", true)) {
// 		std::filesystem::remove(emlPath);
// 	}

// 	return std::to_string(model);
// }