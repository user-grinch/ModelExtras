#pragma once
#include "pch.h"
#include <map>
#include <vector>

template <class T, class Y> class Extender {
private:
  inline static std::vector<std::pair<T, Y>> data{};

public:
  Extender(const Extender &) = delete;
  Extender() {}

  Y &Get(T ptr) {
    for (auto it = data.begin(); it < data.end(); ++it) {
      if (it->first == ptr) {
        return it->second;
      }
    }

    data.push_back({ptr, Y(ptr)});
    return data.back().second;
  }
};