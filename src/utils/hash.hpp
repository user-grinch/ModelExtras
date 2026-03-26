#pragma once
#include <cstdint>
#include <string_view>
#include <cstddef> 

class Hash {
public:
    using Value = uint32_t;

    static constexpr Value OffsetBasis = 0x811c9dc5;
    static constexpr Value Prime       = 0x01000193;

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
        constexpr std::size_t prefixLen = N - 1; 
        
        if (str.size() < prefixLen) {
            return false;
        }
        
        return Get(str.substr(0, prefixLen)) == Get(std::string_view(prefix, prefixLen));
    }
};

constexpr Hash::Value operator""_h(const char* str, std::size_t len) {
    return Hash::Get(std::string_view(str, len));
}