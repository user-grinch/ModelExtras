#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <optional>

class StringUtil
{
public:
    static bool IsNumber(const std::string &s);
    static std::optional<int> GetDigitsAfter(const std::string_view str, const std::string_view prefix);
    static std::optional<std::string> GetCharsAfterPrefix(const std::string_view str, const std::string_view prefix, size_t num_chars);
    static void GetModelsFromIni(std::string &line, std::vector<int> &vec);
};
