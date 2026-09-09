#include "pch.h"
#include "stringutil.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

bool StringUtil::IsNumber(const std::string &s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

std::optional<int> StringUtil::GetDigitsAfter(const std::string_view str, const std::string_view prefix)
{
    if (!str.starts_with(prefix)) return std::nullopt;

    size_t pos = prefix.length();
    if (pos < str.length() && (str[pos] == '_' || str[pos] == '-')) {
        pos++;
    }

    size_t start = pos;
    while (pos < str.length() && std::isdigit(static_cast<unsigned char>(str[pos]))) {
        pos++;
    }

    if (pos == start) return std::nullopt;

    int result = 0;
    for (size_t i = start; i < pos; ++i) {
        result = result * 10 + (str[i] - '0');
    }
    return result;
}

std::optional<std::string> StringUtil::GetCharsAfterPrefix(const std::string_view str, const std::string_view prefix, size_t num_chars)
{
    if (!str.starts_with(prefix)) return std::nullopt;

    size_t pos = prefix.length();
    if (pos < str.length() && (str[pos] == '_' || str[pos] == '-')) {
        pos++;
    }

    if (pos + num_chars <= str.length()) {
        std::string result(str.substr(pos, num_chars));
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return result;
    }
    return std::nullopt;
}

void StringUtil::GetModelsFromIni(std::string &line, std::vector<int> &vec)
{
    if (line.empty()) return;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            item = item.substr(start, end - start + 1);
            try {
                vec.push_back(std::stoi(item));
            } catch (...) {}
        }
    }
}
