#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>

namespace StudioFile {
inline std::string Read(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) return {};
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Could not read " + path.string());
    input.seekg(0, std::ios::end);
    if (input.tellg() > 2 * 1024 * 1024) throw std::runtime_error("Studio file limit: 2 MiB");
    input.seekg(0);
    return {(std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()};
}
inline void Save(const std::filesystem::path &path, const std::string &expected, bool existed, const std::string &value) {
    if (std::filesystem::exists(path) != existed || Read(path) != expected)
        throw std::runtime_error("File changed outside Studio. Reload it before saving.");
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Could not write " + path.string());
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
}
}
