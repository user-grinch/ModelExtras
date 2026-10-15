#pragma once
#include "studio_support.h"
#include "features/lights/manager.h"
#include "features/lights/components/headlight.h"
#include "features/lights/components/indicator.h"
#include "features/lights/components/reverse_light.h"
#include "features/lights/components/brake_light.h"
#include "features/lights/components/tail_light.h"
#include "features/lights/components/stt_light.h"
#include "features/lights/components/nabrake_light.h"
#include "features/lights/components/fog_light.h"
#include "features/lights/components/strobe_light.h"
#include "features/lights/components/drl_light.h"
#include "features/lights/components/side_light.h"
#include "features/lights/components/spot_light.h"
#include "utils/util.h"
#include <tuple>

namespace StudioModel {
using namespace StudioSupport;
inline void Light(Model &out, eMaterialType type, unsigned flags) {
    if (auto *key=LightManager::GetLightSpecificKey(type)) {
        out.Add("lights",key,flags);
        if (flags&Material) out.Add("leds",key,Material); // GetMaterialColor also reads leds.<specific>.
    }
    if (auto *group=LightManager::GetLightGroupKey(type)) {
        out.Add("lights",group,flags);
        if (flags&Material) out.Add("leds",group,Material);
    }
    // Material overrides are exposed under their canonical JSONC paths.
    if (!(flags&Material)) return;
}
inline Model Inspect(CVehicleModelInfo *info, int model, std::vector<std::string> &frames) {
    Model out; frames.clear();
    if (!info || !info->m_pRwClump) return out;
    out.inspected=true;
    out.remaps=static_cast<unsigned>(std::clamp(info->GetNumRemaps(),0,128));
    const bool bike=CModelInfo::IsBikeModel(model);
    const bool mileage=bike || CModelInfo::IsCarModel(model) || CModelInfo::IsMonsterTruckModel(model) || CModelInfo::IsQuadBikeModel(model);
    const bool headlights=!CModelInfo::IsBmxModel(model) && !CModelInfo::IsBoatModel(model) && !CModelInfo::IsTrailerModel(model) && !CModelInfo::IsHeliModel(model) && !CModelInfo::IsPlaneModel(model);
    size_t visited=0;
    const auto visit=[&](auto &&self, RwFrame *frame, unsigned depth)->void {
        for (; frame && visited++<4096; frame=frame->next) {
            auto name=GetSafeFrameNodeName(frame);
            if (!name.empty()) {
                for (auto *section : {"exhausts","roofs","spoilers","doors","gauges","clocks"}) {
                    if (StudioSchema::NodeFields(section,name).empty()) continue;
                    unsigned children=0, clockBits=0; bool digits=false;
                    for (auto *child=frame->child; child && children<4096; child=child->next,++children) {
                        auto childName=GetSafeFrameNodeName(child);
                        if (childName=="digits") {
                            unsigned meshes=0;
                            for (auto *digit=child->child; digit && meshes<10; digit=digit->next,++meshes) {
                                auto *object=GetFirstObject(digit);
                                if (!object || RwObjectGetType(object)!=rpATOMIC) break;
                            }
                            digits=meshes==10;
                        }
                        for (unsigned i=1;i<=4;++i) if (childName=="digit"+std::to_string(i)) clockBits|=1u<<i;
                    }
                    if (std::string_view(section)=="clocks" && (!digits || clockBits!=30)) continue;
                    if ((name.starts_with("x_odometer") || name.starts_with("fc_om")) && (!mileage || children<6)) continue;
                    out.Add(section,std::string(name),Dummy);
                }
                if (name=="x_rb_bed") out.Add("rollback_bed","bed",Rotation);
                if (name=="x_rb_hydraulics") out.Add("rollback_bed","hydraulics",Rotation);
                if (name.starts_with("x_rb_hydraulic_")) out.Add("rollback_bed","hydraulics",Movement);
                if (rwLinkListEmpty(&frame->objectList)) {
                    auto types=DummyTypes(name,frame->modelling.pos.x,frame->modelling.pos.y,bike,headlights);
                    if (!types.empty()) {
                        std::string key(name.substr(0,name.find("_prm")));
                        unsigned flags=Dummy;
                        for (auto type : types) {
                            if (type==StrobeLight) flags|=Strobe;
                            Light(out,type,flags);
                        }
                        out.Add("lights",key,flags);
                    }
                    for (auto *prefix : {"siren_","siren","light_em"}) if (auto id=DigitsAfter(name,prefix)) {
                        if (*id>=0 && *id<=256) out.Add("sirens",std::to_string(*id),Dummy);
                        break;
                    }
                }
                frames.emplace_back(name);
            }
            if (depth<64) self(self,frame->child,depth+1); else if (frame->child) out.limited=true;
        }
    };
    visit(visit,RpClumpGetFrame(info->m_pRwClump),0);
    if (visited>4096) out.limited=true;
    // Reuse the real component material registration even when global lighting is disabled.
    // No Init/TryRegisterDummy calls, gameplay state, geometry changes or hooks.
    static const auto materialMap=[] {
        std::unordered_map<uint32_t,eMaterialType> map;
        std::tuple<HeadlightComponent,IndicatorComponent,ReverseLightComponent,BrakeLightComponent,TailLightComponent,STTLightComponent,NABrakeLightComponent,FogLightComponent,StrobeLightComponent,DRLLightComponent,SideLightComponent,SpotLightComponent> components;
        std::apply([&](auto &...c) { (c.RegisterMaterials(map),...); },components);
        return map;
    }();
    struct Scan { Model *out; unsigned materials=0,atomics=0; bool white=false; } scan{&out};
    RpClumpForAllAtomics(info->m_pRwClump,[](RpAtomic *atomic, void *data)->RpAtomic * {
        auto &s=*static_cast<Scan *>(data);
        if (!atomic || ++s.atomics>4096) return nullptr;
        if (auto *geometry=RpAtomicGetGeometry(atomic)) RpGeometryForAllMaterials(geometry,[](RpMaterial *mat, void *data)->RpMaterial * {
            auto &s=*static_cast<Scan *>(data);
            if (!mat || ++s.materials>16384) return nullptr;
            auto *c=RpMaterialGetColor(mat); if (!c) return mat;
            CRGBA col(c->red,c->green,c->blue,255);
            if (!Util::IsAntiPatternLightMaterial(mat)) {
                if (auto it=materialMap.find(col.ToInt());it!=materialMap.end()) Light(*s.out,it->second,Material);
            }
            if (col.r==255 && col.g==200 && col.b>=100 && col.b<=111) Light(*s.out,static_cast<eMaterialType>(EngineOnLed+col.b-100),Material);
            if (col.r==60 && col.g==255) s.out->paintSlots|=1;
            if (col==VEHCOL_SECONDARY) s.out->paintSlots|=2;
            if (col==VEHCOL_TERTIARY) s.out->paintSlots|=4;
            if (col==VEHCOL_QUATARNARY) s.out->paintSlots|=8;
            if (col.r>0 && col.g==255 && col.b==255) {
                if (col.r==255) s.white=true;
                else s.out->Add("sirens",std::to_string(col.r),Material|StandardSiren);
            }
            if (col.r>=240 && col.g==0 && col.b==0) s.out->Add("sirens",std::to_string(256-col.r),Material|IVFSiren);
            if (auto *tex=mat->texture) {
                if (!_stricmp(tex->name,"carplate") || !_stricmp(tex->name,"carpback") || !_strnicmp(tex->name,"plate_",6)
                    || (tex->raster && tex->raster->width==256 && tex->raster->height==64)) s.out->Add("plate","color",Material);
            }
            return mat;
        },data);
        return atomic;
    },&scan);
    if (scan.white && out.Get("sirens","255")) out.Add("sirens","255",Material|StandardSiren);
    if (scan.atomics>4096 || scan.materials>16384) out.limited=true;
    if (out.limited) { out.entries.clear(); out.paintSlots=0; out.inspected=false; frames.clear(); }
    DeduplicateNames(frames);
    return out;
}
}
