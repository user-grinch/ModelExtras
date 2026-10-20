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
    auto temp = path; temp += ".studio.tmp";
    auto backup = path; backup += ".studio.bak";
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Could not create temporary file");
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
    output.flush();
    if (!output) throw std::runtime_error("Could not write temporary file");
    output.close();
    if (output.fail()) throw std::runtime_error("Could not close temporary file");
    // ReplaceFile creates the backup and replaces atomically on the same volume.
    bool saved = existed
        ? ReplaceFileW(path.c_str(), temp.c_str(), backup.c_str(), 0, nullptr, nullptr) != FALSE
        : MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!saved) throw std::runtime_error("Save failed (Windows error " + std::to_string(GetLastError()) + ")");
}
}
