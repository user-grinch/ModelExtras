#pragma once
#include "pch.h"
#include <vector>
#include <map>

static ThiscallEvent <AddressList<0x5E6342, H_CALL>, PRIORITY_BEFORE, ArgPickN<CWeapon*, 0>, void(CWeapon*)>  weaponDtorEvent;

template <class T>
class WeaponExtender {
private:
    inline static std::vector<std::pair<CWeapon*, T>> data{};

public:
    WeaponExtender(const WeaponExtender&) = delete;

    // Handle Jetpack unload
    WeaponExtender() {
        weaponDtorEvent.before += [](CWeapon *pWeapon) {
            for (auto it = data.begin(); it != data.end(); it++) {
                if (it->first == pWeapon) {
                    data.erase(it);
                    break;
                }
            }
        };
    }

    T& Get(CWeapon* pWeapon) {
        for (auto it = data.begin(); it < data.end(); ++it) {
            if (it->first == pWeapon) {
                return it->second;
            }
        }

        data.push_back({ pWeapon, T(pWeapon) });
        return data.back().second;
    }
};