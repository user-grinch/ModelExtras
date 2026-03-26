#include "pch.h"

#include <CCoronas.h>
#include <CModelInfo.h>
#include <CRadar.h>
#include <CShadows.h>
#include <CStreaming.h>

#include "fla_sa.h"

using namespace plugin;

void FLASupport::Install()
{
    // CoronasLimitSupport
    // MAX_CORONAS = patch::GetUInt(0x6FAF46 + 4);
    // CCoronas__aCoronas =
    // reinterpret_cast<CRegisteredCorona*>(patch::GetPointer(0x6FAE9F + 1));

    // // ShadowsLimitSupport
    // MAX_STORED_SHADOWS =
    // patch::GetUShort(injector::GetBranchDestination(0x707398).as_int() + 3);
    // CShadows__asShadowsStored =
    // reinterpret_cast<CRegisteredShadow*>(patch::GetPointer(0x7073B0 + 1));

    // // ModelInfoLimitSupport
    CModelInfo__ms_modelInfoPtrs = reinterpret_cast<CBaseModelInfo **>(patch::GetPointer(0x403DA7));
    CStreaming__ms_aInfoForModel = reinterpret_cast<CStreamingInfo *>(patch::GetPointer(0x408ADA + 3));

    // RadarBlipsLimitSupport
    CRadar__RadarBlipSprites = reinterpret_cast<CSprite2d *>(patch::GetPointer(0x5827EA + 1));
    MAX_RADAR_SPRITES = (patch::GetUInt(0x585950 + 2) - reinterpret_cast<unsigned int>(CRadar__RadarBlipSprites)) / 4;
    CRadar__ms_RadarTrace = reinterpret_cast<tRadarTrace *>(patch::GetPointer(0x5838B0 + 2));
    MAX_RADAR_TRACES = patch::GetUInt(0x58384C + 2);
    CRadar__MapLegendList = reinterpret_cast<unsigned short *>(patch::GetPointer(0x585A52 + 1));
}
