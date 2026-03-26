#pragma once
#include <cstdint>
#include <string_view>
#include <cstddef> // Required for std::size_t

class Hash {
public:
    using Value = uint32_t;

    static constexpr Value OffsetBasis = 0x811c9dc5;
    static constexpr Value Prime       = 0x01000193;

    // Core hashing logic
    static constexpr Value Get(std::string_view str) {
        Value hash = OffsetBasis;
        for (unsigned char c : str) {
            hash ^= static_cast<Value>(c);
            hash *= Prime;
        }
        return hash;
    }

    template<std::size_t N>
    static constexpr bool StartsWith(std::string_view str, const char (&prefix)[N]) {
        constexpr std::size_t prefixLen = N - 1; // get rid of the null terminator
        
        if (str.size() < prefixLen) {
            return false;
        }
        
        // Note: The right side evaluates at runtime. 
        // For safety and speed, it is much better to just write:
        // return str.substr(0, prefixLen) == std::string_view(prefix, prefixLen);
        return Get(str.substr(0, prefixLen)) == Get(std::string_view(prefix, prefixLen));
    }
};

// Removed the space and explicitly used std::size_t
constexpr Hash::Value operator""_h(const char* str, std::size_t len) {
    return Hash::Get(std::string_view(str, len));
}