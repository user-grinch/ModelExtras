#pragma once

class CRegisteredCorona;
class CRegisteredShadow;
class CBaseModelInfo;
class CStreamingInfo;
class CSprite2d;
class tRadarTrace;

class FLASupport
{
  public:
    // CoronasLimitSupport
    static inline unsigned int MAX_CORONAS;
    static inline CRegisteredCorona *CCoronas__aCoronas;

    // ShadowsLimitSupport
    static inline unsigned int MAX_STORED_SHADOWS;
    static inline CRegisteredShadow *CShadows__asShadowsStored;

    // ModelInfoLimitSupport
    static inline CBaseModelInfo **CModelInfo__ms_modelInfoPtrs;
    static inline CStreamingInfo *CStreaming__ms_aInfoForModel;

    // RadarBlipsLimitSupport
    static inline CSprite2d *CRadar__RadarBlipSprites;
    static inline unsigned int MAX_RADAR_SPRITES;
    static inline tRadarTrace *CRadar__ms_RadarTrace;
    static inline unsigned int MAX_RADAR_TRACES;
    static inline unsigned short *CRadar__MapLegendList;

    static void Install();
};
