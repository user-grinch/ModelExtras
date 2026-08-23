set_project("ModelExtras")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
set_defaultmode("release")

local PLUGIN_SDK_DIR = os.getenv("PLUGIN_SDK_DIR")
local GAME_DIR = os.getenv("GTASA_DIR")

target("ModelExtras")
    set_kind("shared")
    set_extension(".asi")
    set_languages("c++latest")
    set_arch("x86")
    set_plat("windows")
    set_filename("ModelExtras.asi")

    set_pcxxheader("src/pch.h")

    add_defines(
        "PLUGIN_SGV_10US", 
        "MODELEXTRAS_DEV", 
        "GTASA", 
        "RW",
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN",
        "_USE_MATH_DEFINES"
    )

    if PLUGIN_SDK_DIR then
        add_includedirs(
            PLUGIN_SDK_DIR,
            path.join(PLUGIN_SDK_DIR, "plugin_sa"),
            path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa"),
            path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa", "enums"),
            path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa", "meta"),
            path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa", "rw"),
            path.join(PLUGIN_SDK_DIR, "shared"),
            path.join(PLUGIN_SDK_DIR, "shared", "game"),
            path.join(PLUGIN_SDK_DIR, "injector"),
            path.join(PLUGIN_SDK_DIR, "safetyhook")
        )

        add_linkdirs(
            path.join(PLUGIN_SDK_DIR, "output", "lib")
        )
    end

    add_includedirs(
        ".",
        "include", 
        "include/coreutils", 
        "src", 
        "src/features"
    )

    add_files(
        "src/**.cpp"
    )

    add_syslinks(
        "dwmapi", 
        "shell32", 
        "gdi32", 
        "user32", 
        "advapi32"
    )

    set_warnings("all")

    add_cxflags(
        "/permissive-",
        "/Zc:__cplusplus",
        "/Zc:preprocessor",
        "/utf-8",
        "/EHsc",
        "/Gw",
        "/Zc:threadSafeInit-",
        "/MP"
    )

    add_shflags(
        "/SAFESEH:NO",
        "/FORCE:MULTIPLE",
        "/LARGEADDRESSAWARE"
    )

    if is_mode("debug") then
        add_links("plugin_d")
        set_runtimes("MTd")
        set_optimize("none")
        set_symbols("debug")
    else
        add_links("plugin")
        set_runtimes("MT")
        set_optimize("fastest")
        set_symbols("hidden")
    end

    after_build(function (target)
        if GAME_DIR then
            os.cp(target:targetfile(), GAME_DIR)
            print(">> Deployed to Game Directory")
        end
    end)
