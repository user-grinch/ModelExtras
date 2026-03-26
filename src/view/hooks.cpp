#include "pch.h"
#include "imgui/rw/renderhook.h"
#include <extensions/ScriptCommands.h>
#include <imgui/fonts/fontmgr.h>
#include <imgui/fonts/fonts.h>

#define TEST_CHEAT 0x0ADC

using namespace plugin;

extern void DevWindow();

bool bWindowOpenFlag = false;
void InjectImGuiHooks() {
    ImGui::CreateContext();
    Events::initGameEvent.after += []()
    {
        RenderHook::Init(DevWindow);
    };

    Events::processScriptsEvent.after += []()
    {
        static float screenX = -1;
        static float screenY = -1;
        if (screenX != SCREEN_WIDTH || screenY != SCREEN_HEIGHT) {
            if (screenX == -1 && screenY == -1)
            {
                auto& io = ImGui::GetIO();
                io.FontDefault = FontMgr::LoadFont("text", textFont, 28.0f);
                FontMgr::LoadFont("title", titleFont, 40.0f);
            }

            FontMgr::RescaleFonts(SCREEN_WIDTH, SCREEN_HEIGHT);
            screenX = SCREEN_WIDTH;
            screenY = SCREEN_HEIGHT;
        }

        if (Command<TEST_CHEAT>("MEDEV")) {
            bWindowOpenFlag = !bWindowOpenFlag;
            RenderHook::SetCursorVisible(bWindowOpenFlag);
        }
    };
}