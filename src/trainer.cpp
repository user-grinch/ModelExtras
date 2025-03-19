#include "pch.h"
#include "GrinchTrainerAPI.h"

extern bool gbGlobalIndicatorLights;
extern bool gbGlobalReverseLights;
extern float gfGlobalCoronaSize;
extern int gGlobalCoronaIntensity;
extern int gGlobalShadowIntensity;

#define F_NULL [](void *) {}

void TrainerInit()
{
    TAPI_ClearWidgets();
    if (TAPI_InitConnect("ModelExtras", TAPI_VERSION) == TReturn_Success)
    {
        static const char *tabs[] = {"Features", "Vehicles", "Weapons"};
        static size_t selectedTab = 0;
        TAPI_Spacing(0, 10);
        if (TAPI_Tabs(tabs, 3, &selectedTab) == TReturn_Success)
        {
            if (selectedTab == 0)
            {
                TAPI_Spacing(0, 10);
                TAPI_Columns(2);
                TAPI_Checkbox("Global Indicator Lights", &gbGlobalIndicatorLights, F_NULL);
                TAPI_NextColumn();
                TAPI_Checkbox("Global Reverse Lights", &gbGlobalReverseLights, F_NULL);
                TAPI_Columns(1);

                TAPI_Spacing(0, 10);

                TAPI_InputInt("Global Corona Intensity", &gGlobalCoronaIntensity, F_NULL, 0, 255);
                TAPI_InputFloat("Global Corona Size", &gfGlobalCoronaSize, F_NULL, 0.0f, 10.0f);
                TAPI_InputInt("Global Shadow Intensity", &gGlobalShadowIntensity, F_NULL, 0, 255);
            }

            if (selectedTab == 1)
            {
                TAPI_Text("Vehicles TAB");
            }

            if (selectedTab == 2)
            {
                TAPI_Text("Weapons TAB");
            }
        }
        TAPI_CloseConnect();
    }
}