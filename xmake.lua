set_project("ModelExtras")

add_rules("mode.debug", "mode.release")
set_defaultmode("release")

local PLUGIN_SDK_DIR = os.getenv("PLUGIN_SDK_DIR")
if not (PLUGIN_SDK_DIR and os.isdir(PLUGIN_SDK_DIR)) then
    for _, dir in ipairs({"plugin-sdk", "../plugin-sdk", "../../plugin-sdk"}) do
        if os.isdir(dir) then
            PLUGIN_SDK_DIR = path.absolute(dir)
            break
        end
    end
end

local GAME_DIR = os.getenv("GTASA_DIR") or "D:/Games/GTA San Andreas"

target("ModelExtras")
    set_kind("shared")
    set_filename("ModelExtras.asi")
    set_languages("c++latest")
    set_arch("x86")
    set_plat("windows")

    set_pcxxheader("src/pch.h")

    add_defines(
        "PLUGIN_SGV_10US", 
        "MODELEXTRAS_DEV", 
        "GTASA", 
        "RW",
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN",
        "_USE_MATH_DEFINES",
        "IMGUI_DISABLE_DEBUG_TOOLS",
        "IMGUI_DISABLE_DEFAULT_FONT_BITMAP",
        "IMGUI_USE_BGRA_PACKED_COLOR",
        "IMGUI_IMPL_WIN32_DISABLE_GAMEPAD",
        "IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS",
        "IMGUI_DISABLE_OBSOLETE_FUNCTIONS"
    )

    add_includedirs(
        PLUGIN_SDK_DIR,
        path.join(PLUGIN_SDK_DIR, "plugin_sa"),
        path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa"),
        path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa", "enums"),
        path.join(PLUGIN_SDK_DIR, "plugin_sa", "game_sa", "rw"),
        path.join(PLUGIN_SDK_DIR, "shared"),
        path.join(PLUGIN_SDK_DIR, "shared", "game"),
        "include", 
        "include/imgui",
        "src", 
        "src/features"
    )

    add_linkdirs(
        path.join(PLUGIN_SDK_DIR, "output", "lib")
    )

    add_files(
        "src/**.cpp"
    )

    add_syslinks(
        "user32", "d3d9", "dwmapi"
    )

    set_warnings("all")

    add_cxflags(
        "/options:strict",
        "/Zc:__cplusplus",
        "/Zc:preprocessor",
        "/utf-8",
        "/EHsc",
        "/Gw",
        "/Zc:threadSafeInit-",
        "/Zc:inline",
        "/Zc:throwingNew",
        "/Zc:templateScope",
        "/volatile:iso",
        "/Qfast_transcendentals",
        "/fp:contract",
        "/fpcvt:IA",
        "/Zc:enumTypes",
        "/Zc:enumEncoding",
        "/Zc:checkGwOdr"
    )

    add_shflags(
        "/SAFESEH:NO",
        "/LARGEADDRESSAWARE",
        "/DYNAMICBASE:NO",
        "/NXCOMPAT:NO",
        "/MANIFEST:NO"
    )

    if is_mode("debug") then
        add_links("plugin_d")
        set_runtimes("MTd")
    else
        add_links("plugin")
        set_runtimes("MT")
        set_optimize("fastest")
        set_symbols("hidden")
        add_defines("_ITERATOR_DEBUG_LEVEL=0")
        add_cxflags("/GL", "/Zo", "/Ob3", "/Oy-")
        add_shflags("/INCREMENTAL:NO", "/LTCG", "/OPT:REF,ICF", "/RELEASE")
    end

    after_build(function (target)
        if GAME_DIR and os.isdir(GAME_DIR) then
            os.cp(target:targetfile(), GAME_DIR)
            print(">> Deployed .asi to " .. GAME_DIR)

            local symbolfile = target:symbolfile()
            if symbolfile and os.isfile(symbolfile) then
                os.cp(symbolfile, GAME_DIR)
                print(">> Deployed .pdb to " .. GAME_DIR)
            end

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
            {'m', "mode", "kv", "debug", "Set build mode: debug, release"}
        }
    }
    on_run(function ()
        import("core.base.option")
        local mode = option.get("mode") or "debug"
        os.execv("xmake", {"f", "-p", "windows", "-a", "x86", "-m", mode, "-y"})
        os.execv("xmake", {})
        os.execv("xmake", {"project", "-k", "compile_commands"})
    end)
