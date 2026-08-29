set_project("ModelExtras")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
set_defaultmode("release")

local PLUGIN_SDK_DIR = os.getenv("PLUGIN_SDK_DIR")
local GAME_DIR = os.getenv("GTASA_DIR") or "D:/Games/GTA San Andreas"

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
        "src", 
        "src/features"
    )

    add_files(
        "src/**.cpp"
    )

    add_syslinks(
        "user32"
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
        if GAME_DIR and os.isdir(GAME_DIR) then
            -- Copy target (.asi)
            os.cp(target:targetfile(), GAME_DIR)
            print(">> Deployed .asi to " .. GAME_DIR)

            -- Copy debug symbols (.pdb) if available
            local symbolfile = target:symbolfile()
            if symbolfile and os.isfile(symbolfile) then
                os.cp(symbolfile, GAME_DIR)
                print(">> Deployed .pdb to " .. GAME_DIR)
            end

            -- Copy resources (ModelExtras folder, ModelExtras.ini)
            if os.isdir("resource/dist") then
                os.cp("resource/dist/*", GAME_DIR)
                print(">> Deployed resources to " .. GAME_DIR)
            end
        end
    end)

task("dev")
    set_menu {
        usage = "xmake dev [options]",
        description = "Configure and build ModelExtras",
        options = {
            {'m', "mode", "kv", "debug", "Set build mode: debug, release, releasedbg"}
        }
    }
    on_run(function ()
        import("core.base.option")
        local mode = option.get("mode") or "debug"
        os.execv("xmake", {"f", "-p", "windows", "-a", "x86", "-m", mode, "-y"})
        os.execv("xmake", {})
        os.execv("xmake", {"project", "-k", "compile_commands"})
    end)

