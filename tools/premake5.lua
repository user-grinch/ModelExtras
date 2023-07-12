PSDK_DIR = os.getenv("PLUGIN_SDK_DIR")

workspace "FunctionalComponents"
    configurations { 
        "Debug", 
        "Release" 
    }

    architecture "x86"
    platforms "Win32"
    language "C++"
    cppdialect "C++20"
    characterset "MBCS"
    staticruntime "On"
    location "../build"
    targetdir "../build/bin"

    linkoptions {
        "/SAFESEH:NO", -- Image Has Safe Exception Handers: No
    }

-- project "depend"
--     kind "StaticLib"

--     files { 
--         "../include/**",
--     }

--     filter "configurations:Debug"
--         defines { "DEBUG" }
--         symbols "On"

--     filter "configurations:Release"
--         defines { "NDEBUG" }
--         optimize "On"

project "FunctionalComponents"
    kind "SharedLib"
    targetextension ".asi"

    -- links { 
    --     "depend",
    -- }

    files { 
        "../src/**",
    }

    pchheader "pch.h"
    pchsource "../src/pch.cpp"

    defines { 
        "IS_PLATFORM_WIN" ,
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NON_CONFORMING_SWPRINTFS",
        "_DX9_SDK_INSTALLED",
        "PLUGIN_SGV_10US",
        "_GTA_",
        "GTASA",
    }
    
    includedirs {
        "../include/",
        PSDK_DIR .. "/plugin_sa/",
        PSDK_DIR .. "/plugin_sa/game_sa/",
        PSDK_DIR .. "/shared/",
        PSDK_DIR .. "/shared/game/"
    }
    
    libdirs {
        PSDK_DIR .. "/output/lib",
        "build/bin/",
        "../lib/"
    }

    filter "configurations:Debug"
        symbols "On"
        links { 
            -- "depend",
            "plugin_d.lib",
            "bass",
        }

    filter "configurations:Release"
        optimize "On"
        links { 
            -- "depend",
            "plugin.lib",
            "bass",
        }