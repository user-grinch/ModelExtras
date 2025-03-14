require "ecc/ecc"

PSDK_DIR = os.getenv("PLUGIN_SDK_DIR")

workspace "ModelExtras"
    configurations { 
        "Debug", 
        "Release" 
    }

    architecture "x86"
    platforms "Win32"
    language "C++"
    cppdialect "C++latest"
    characterset "MBCS"
    staticruntime "On"
    location "../build"
    targetdir "../build/bin"

    linkoptions {
        "/SAFESEH:NO",
    }

project "ModelExtras"
    kind "SharedLib"
    targetextension ".asi"

    files { 
        "../src/**",
    }

    pchheader "pch.h"
    pchsource "../src/pch.cpp"

    defines { 
        "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
        "IS_PLATFORM_WIN" ,
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NON_CONFORMING_SWPRINTFS",
        "_DX9_SDK_INSTALLED",
        "PLUGIN_SGV_10US",
        "_GTA_",
        "GTASA",
        "RW"
    }

    includedirs {
        "../include/",
        "../src/",
        PSDK_DIR .. "/*",
        PSDK_DIR .. "/plugin_sa/*",
        PSDK_DIR .. "/plugin_sa/game_sa/*",
        PSDK_DIR .. "/plugin_sa/game_sa/rw/*",
        PSDK_DIR .. "/shared/*",
        PSDK_DIR .. "/shared/game/*",
    }
    
    libdirs {
        PSDK_DIR .. "/output/lib",
        "build/bin/",
        "../lib/"
    }

    filter "configurations:Debug"
        symbols "On"
        links { 
            "plugin_d.lib",
            "bass",
        }

    filter "configurations:Release"
        optimize "On"
        links { 
            "plugin.lib",
            "bass",
        }