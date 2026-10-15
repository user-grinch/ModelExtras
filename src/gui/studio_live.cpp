#include "pch.h"
#include "studio_live.h"
#include "studio_policy.h"
#include "loader.h"
#include "features/lights/manager.h"
#include "features/exhausts.h"
#include "features/gauge.h"
#include "features/clock.h"
#include "features/roof.h"
#include "features/spoiler.h"
#include "features/slidedoor.h"
#include "features/rotatedoor.h"
#include "features/rollbackbed.h"
#include "features/carcols.h"
#include "features/sirens.h"
#include "features/soundeffects.h"
#include "utils/audiomgr.h"
#include "utils/datamgr.h"
#include "utils/render.h"

namespace {
using Json = nlohmann::json;
const Json &Section(const Json &json, const char *key) {
    static const auto empty = Json::object();
    auto it = json.find(key);
    return it != json.end() && it->is_object() ? *it : empty;
}
const Json &Node(const Json &json, RwFrame *frame) {
    static const auto empty = Json::object();
    if (!frame) return empty;
    auto it = json.find(GetSafeFrameNodeName(frame));
    return it != json.end() && it->is_object() ? *it : empty;
}
void RefreshLights(CVehicle *vehicle) {
    for (auto &group : LightManager::m_VehData.Get(vehicle).dummies)
        for (auto &light : group) light.RefreshConfig();
}
void RefreshFixedGauge(CVehicle *vehicle, const Json &gauges) {
    for (auto &entry : FixedGauge::m_VehData.Get(vehicle).gauges) {
        if (!entry.frame) continue;
        const auto &json = Node(gauges, entry.frame);
        float min = json.value("minangle", 30.0f), max = json.value("maxangle", 120.0f);
        float angle = CBaseFeature::IsEnabled(eFeatureMatrix::AnimatedGasMeter) ? min + entry.fraction * (max - min) : 0.0f;
        FrameUtil::SetRotationY(entry.frame, angle - entry.angle);
        entry.angle = angle;
    }
}
template <typename T> void RefreshDoors(T &data, const Json &json, const char *key, float popout) {
    const auto update = [&](auto &group) {
        for (auto &entry : group) {
            if (!entry.frame) continue;
            const auto &node = Node(json, entry.frame);
            entry.mul = node.value(key, 1.0f);
            entry.popOutAmount = node.value("popout", json.contains(GetSafeFrameNodeName(entry.frame)) ? 0.15f : popout);
        }
    };
    update(data.leftFront); update(data.rightFront); update(data.leftRear); update(data.rightRear);
    if constexpr (requires { data.boot; }) { update(data.boot); update(data.bonnet); }
}
}

void StudioLive::ApplyIni(const char *section, const char *key) {
    if (StudioPolicy::RestartReason(section, key)) return;
    // Refresh caches without reloading files or tearing down vehicle features.
    if (std::string_view(section) == "LIGHTS") {
        LightsConfig::Get().InitConfig();
        RenderUtil::ReloadConfig();
        ModelInfoMgr::ReloadConfig();
        for (auto &feature : ModelExtras::m_Features)
            if (feature && (feature->GetName() == key || feature->GetName() == "SirenLights" || feature->GetName() == "StandardLights")) feature->ReloadConfig();
        const std::string_view setting(key);
        if (CPools::ms_pVehiclePool && (setting.ends_with("Size") || setting.ends_with("Intensity")))
            for (auto *vehicle : CPools::ms_pVehiclePool)
                if (vehicle && vehicle->m_pRwClump) RefreshLights(vehicle);
    } else if (std::string_view(section) == "SOUND") {
        AudioMgr::ReloadConfig();
        if (std::string_view(key) != "SoundMult") for (auto &feature : ModelExtras::m_Features)
            if (feature && feature->GetName() == "SoundEffects") feature->ReloadConfig();
    } else {
        ModelExtras::ReloadConfig();
        for (auto &feature : ModelExtras::m_Features)
            if (feature && (std::string_view(key).starts_with(feature->GetName()) || std::string_view(section) == "KEYS" || std::string_view(section) == "TABLE")) feature->ReloadConfig();
    }
}

void StudioLive::ApplyModel(int model, const Json &before, const Json &after) {
    const auto changed = [&](const char *key) {
        auto a = before.find(key), b = after.find(key);
        return a == before.end() ? b != after.end() : b == after.end() || *a != *b;
    };
    if (changed("sirens")) DataMgr::NotifyChanged(model, "sirens");
    if (changed("carcols")) DataMgr::NotifyChanged(model, "carcols");
    if (!CPools::ms_pVehiclePool) return;
    for (auto *vehicle : CPools::ms_pVehiclePool) {
        if (!vehicle || vehicle->m_nModelIndex != model) continue;
        if (changed("carcols")) {
            auto &data = Carcols::m_VehData.Get(vehicle);
            const auto &cols = Section(after, "carcols");
            if (!cols.contains("variations") || data.randId >= static_cast<int>(cols.at("variations").size())) data.randId = -1;
            data.m_bPri = data.m_bSec = data.m_bTer = data.m_bQuat = false;
        }
        if (changed("sirens")) {
            auto &data = Sirens::m_VehData.Get(vehicle);
            if (vehicle->m_pRwClump) for (auto *dummy : data.ActiveRotators) if (dummy) dummy->ResetAngle();
            data.ActiveRotators.clear();
            data.State = 0;
            data.Delay = 0;
            if (data.m_nSirenStream) AudioMgr::StopSirenStream(data.m_nSirenStream);
            data.m_nSirenStream = 0;
            data.m_nActiveSirenSoundState = -1;
            data.m_bPlayingCustomSiren = false;
            Sirens::EnsureDummies(vehicle);
        }
        if (changed("sound")) {
            auto &data = SoundEffects::m_VehData.Get(vehicle);
            data.m_bInitialized = false;
        }
        if (!vehicle->m_pRwClump) continue;
        if (changed("lights")) RefreshLights(vehicle);
        if (changed("exhausts")) ExhaustFx::RefreshConfig(vehicle);
        if (changed("clocks")) {
            auto &data = DigitalClockFeature::m_VehData.Get(vehicle);
            data.m_b12HourFormat = Node(Section(after, "clocks"), data.m_pRootFrame).value("12hformat", false);
        }
        if (changed("doors")) {
            const auto &json = Section(after, "doors");
            RefreshDoors(SlideDoor::m_VehData.Get(vehicle), json, "movmul", 0.15f);
            RefreshDoors(RotateDoor::m_VehData.Get(vehicle), json, "mul", 0.0f);
        }
        if (changed("roofs")) {
            auto &data = ConvertibleRoof::m_VehData.Get(vehicle);
            const auto update = [&](auto &group) {
                for (auto &entry : group) {
                    const auto &json = Node(Section(after, "roofs"), entry.pFrame);
                    entry.targetRot = json.value("rotation", 60.0f);
                    entry.speed = json.value("speed", 1.5f);
                }
            };
            update(data.m_Roofs); update(data.m_Boots);
        }
        if (changed("spoilers")) for (auto &entry : Spoiler::m_VehData.Get(vehicle).m_Spoilers) {
            if (!entry.m_pFrame) continue;
            const auto &json = Node(Section(after, "spoilers"), entry.m_pFrame);
            auto name = GetSafeFrameNodeName(entry.m_pFrame);
            float rotation = 3.0f, time = 3000.0f;
            auto first = name.find('_'), last = name.rfind('_');
            if (first != name.npos && last > first) {
                try { rotation = std::stof(std::string(name.substr(first + 1, last - first - 1))); } catch (...) {}
            }
            if (last != name.npos) {
                try { time = std::stof(std::string(name.substr(last + 1))); } catch (...) {}
            }
            entry.m_fRotation = json.value("rotation", rotation);
            entry.m_nTime = json.value("time", time);
            entry.m_nTriggerSpeed = json.value("triggerspeed", 20.0f);
        }
        if (changed("gauges")) {
            const auto &gauges = Section(after, "gauges");
            for (auto &[name, entry] : RPMGauge::m_VehData.Get(vehicle).vecGaugeData) {
                const auto &json = Section(gauges, name.c_str());
                entry.iMaxRPM = json.value("maxrpm", 8000);
                entry.fMaxRotation = json.value("maxrotation", 260.0f);
            }
            for (auto &[name, entry] : SpeedGauge::m_VehData.Get(vehicle).vecGaugeData) {
                const auto &json = Section(gauges, name.c_str());
                entry.iMaxSpeed = json.value("maxspeed", 240);
                entry.fMaxRotation = json.value("maxrotation", 260.0f);
                entry.fMul = json.value("kph", true) ? 1.0f : (1.0f / 1.609f);
            }
            for (auto &[name, entry] : TurboGauge::m_VehData.Get(vehicle).vecGaugeData) {
                const auto &json = Section(gauges, name.c_str());
                entry.iMaxTurbo = json.value("maxturbo", 220.0f);
                entry.fMaxRotation = json.value("maxrotation", 220.0f);
            }
            for (auto &[name, entry] : MileageIndicator::m_VehData.Get(vehicle).vecIndicatorData)
                entry.fMul = Section(gauges, name.c_str()).value("kph", true) ? 160.9f : 1.0f;
            RefreshFixedGauge(vehicle, gauges);
        }
        if (changed("rollback_bed")) {
            auto &data = RollbackBed::m_VehData.Get(vehicle);
            const auto &json = Section(after, "rollback_bed");
            const auto &hydraulics = Section(json, "hydraulics"), &bed = Section(json, "bed");
            data.fHyTargetRot = hydraulics.value("target_rot", 0.0f);
            data.fHyRotSpeed = hydraulics.value("rot_speed", 1.0f);
            data.fGlobalMoveSpeed = hydraulics.value("move_speed", 1.0f);
            for (auto &piston : data.m_Pistons) piston.fTargetMove = hydraulics.value("target_move", 2.0f);
            data.fBedTargetRot = bed.value("target_rot", 0.0f);
            data.fBedRotSpeed = bed.value("rot_speed", 1.0f);
        }
    }
}
