#pragma once
#include <vector>
#include <algorithm>
#include <plugin.h>
#include "utils/meevents.h"


template <class T>
class WeaponExtender {
private:
    struct Entry {
        CWeapon* pWeapon;
        T data;
    };
    
    static inline std::vector<Entry> data{};

public:
    WeaponExtender(const WeaponExtender&) = delete;

    WeaponExtender() {
        data.reserve(32); 

        MEEvents::weaponDtorEvent.before += [](CWeapon* pWeapon) {
            auto it = std::find_if(data.begin(), data.end(), 
                [pWeapon](const Entry& e) { return e.pWeapon == pWeapon; });
            
            if (it != data.end()) {
                if (it != data.end() - 1) {
                    *it = std::move(data.back());
                }
                data.pop_back();
            }
        };
    }

    T& Get(CWeapon* pWeapon) {
        for (auto& entry : data) {
            if (entry.pWeapon == pWeapon) {
                return entry.data;
            }
        }

        return data.emplace_back(pWeapon, T(pWeapon)).data;
    }
};