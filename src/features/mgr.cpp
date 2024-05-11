#include "pch.h"
#include "mgr.h"
#include "vehicle/chain.h"
#include "vehicle/brakes.h"
#include "vehicle/gear.h"
#include "vehicle/meter.h"

FeatureManager FeatureMgr;

FeatureManager::FeatureManager() {
    m_FunctionTable["x_chain"] = m_FunctionTable["fc_chain"] = Chain::Process;
    m_FunctionTable["x_fbrake"] = m_FunctionTable["fc_fbrake"] = FrontBrake::Process;
    m_FunctionTable["x_rbrake"] = m_FunctionTable["fc_rbrake"] = RearBrake::Process;
    m_FunctionTable["x_clutch"] = m_FunctionTable["fc_cl"] = Clutch::Process;
    m_FunctionTable["x_gearlever"] = m_FunctionTable["fc_gl"] = GearLever::Process;
    m_FunctionTable["x_gs"] = GearSound::Process;
    m_FunctionTable["x_gearmeter"] = m_FunctionTable["fc_gm"] = GearMeter::Process;
    m_FunctionTable["x_ometer"] = m_FunctionTable["fc_om"] = OdoMeter::Process;
    m_FunctionTable["x_rpm"] = m_FunctionTable["fc_rpm"] = RpmMeter::Process;
    m_FunctionTable["x_sm"] = m_FunctionTable["fc_sm"] = m_FunctionTable["speedook"] = SpeedMeter::Process;
    m_FunctionTable["x_tm"] = m_FunctionTable["tahook"] = TachoMeter::Process;
    m_FunctionTable["x_gm"] = m_FunctionTable["petrolok"] = GasMeter::Process;
}

static std::string GetNodeName(const std::string& input) {
    int c = 0;
    std::string result;
    
    for (char c : input) {
        if (c == '_') {
            c++;
            if (c == 2) {
                break;
            }
        }
        result += c;
    }
    
    return result;
}

void FeatureManager::FindNodes(RwFrame * frame, CEntity* pEntity) {
    if(frame) {
        const std::string name = GetFrameNodeName(frame);

        if (m_FunctionTable.find(GetNodeName(name)) != m_FunctionTable.end()) {
            m_ModelTable[pEntity->m_nModelIndex].emplace_back(frame, name);
        }

        if (RwFrame * newFrame = frame->child) {
            FindNodes(newFrame, pEntity);
        }
        if (RwFrame * newFrame = frame->next) {
            FindNodes(newFrame, pEntity);
        }
    }
    return;
}

void FeatureManager::Initialize(CEntity *pEntity, RwFrame* frame) {
    int model = pEntity->m_nModelIndex;

    if (m_ModelTable.find(model) == m_ModelTable.end()) {
        FindNodes(frame, pEntity);
    }
}

void FeatureManager::Process(CEntity *pEntity) {
    int model = pEntity->m_nModelIndex;

    for (auto e: m_ModelTable[model]) {
        m_FunctionTable[e.id](e.ptr, pEntity);
    }
}

