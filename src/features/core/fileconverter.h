#pragma once
#include <string>
#include <istream>
#include <nlohmann/json.hpp>

int Convert_EmlToJsonc(const std::string &emlPath);
void Convert_JsonToJsonc(const std::string &inPath);
int Convert_IvfcToJsonc(const std::string &inPath);

bool Parse_EmlToMemory(std::istream &infile, nlohmann::json &jsonData, int &outModel);
bool Parse_AvsJsonToMemory(std::istream &infile, nlohmann::json &jsonData);
bool Parse_IvfcToMemory(std::istream &infile, nlohmann::json &jsonData, int &outModel);
