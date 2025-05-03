PSDK_DIR = os.getenv("PLUGIN_SDK_DIR")

function addIncludeDirs(root)
    for _, dir in ipairs(os.matchdirs(root .. "/*")) do
        includedirs { dir }
        addIncludeDirs(dir)
    end
end

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
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NON_CONFORMING_SWPRINTFS",
        "PLUGIN_SGV_10US",
        "GTASA",
        "RW"
    }

    addIncludeDirs( "../src/")

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

    links {
        "delayimp",
        "GrinchTrainerSA",
    }

    linkoptions { 
        "/SAFESEH:NO",
        "/DELAYLOAD:GrinchTrainerSA.asi"
    }

    filter "configurations:Debug"
        symbols "On"
        links { 
            "plugin_d.lib",
        }

    filter "configurations:Release"
        optimize "On"
        links { 
            "plugin.lib",
        }