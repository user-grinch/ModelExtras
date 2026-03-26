set_project("ModelExtras")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
set_defaultmode("debug")
add_rules("plugin.compile_commands.autoupdate", {
    outputdir = os.projectdir(), 
    lsp = "clangd"
})

local PLUGIN_SDK_DIR = "/mnt/p2/Code/plugin-sdk-mingw64/"--os.getenv("PLUGIN_SDK_DIR")
local GAME_DIR = "/mnt/p2/Games/GTA San Andreas/"

target("ModelExtras")
    set_kind("shared")
    set_extension(".asi")
    set_languages("c++23")
    set_arch("x86")
    set_toolchains("clang")
    set_plat("mingw")
    set_runtimes("static")
    set_filename("ModelExtras.asi")

    set_pcxxheader("src/pch.h")

    add_defines(
        "PLUGIN_SGV_10US", 
        "MODELEXTRAS_DEV", 
        "GTASA", 
        "RW"
    )

    add_includedirs(
        "include", 
        "include/imgui", 
        "include/coreutils", 

        PLUGIN_SDK_DIR,
        PLUGIN_SDK_DIR .. "/plugin_sa/",
        PLUGIN_SDK_DIR .. "/plugin_sa/game_sa/",
        PLUGIN_SDK_DIR .. "/plugin_sa/game_sa/rw/",
        PLUGIN_SDK_DIR .. "/shared/",
        PLUGIN_SDK_DIR .. "/shared/game/",
        PLUGIN_SDK_DIR .. "/injector/",
        PLUGIN_SDK_DIR .. "/safetyhook/",

        "src/", 
        "src/features"
    )
    
    add_linkdirs(
        PLUGIN_SDK_DIR .. "/output/lib"
    )

    add_files(
        "src/**.cpp",
        "include/coreutils/imgui/rw/**.cpp",
        "include/coreutils/imgui/fonts/**.cpp",
        "include/imgui/**.cpp"
    )

    add_cxflags(
        "--target=i686-w64-mingw32", 
        "-fpermissive", 
        "-fcommon", 
        "-fms-extensions", 
        "-Wno-microsoft-include",
        "-gdwarf-4",            
        "-gstrict-dwarf",       
        "-gcodeview",
        "-fdebug-macro"        
    )
    
    add_shflags(
        "-static", 
        "-static-libgcc", 
        "-static-libstdc++", 
        "-Wl,-Bstatic", 
        "-lstdc++", 
        "-lpthread", 
        "-Wl,-Bdynamic", 
        "-Wl,--allow-multiple-definition",
        "-Wl,--build-id",
        {force = true}
    )
    
    add_syslinks(
        "dwmapi", 
        "shell32", 
        "gdi32", 
        "ntdll", 
        "user32", 
        "advapi32"
    )

    after_build(function (target)
        os.cp(target:targetfile(), GAME_DIR)
        print(">> Deployed to Game Directory")
    end)

    if is_mode("debug") then
        add_links("plugin_d")
        set_optimize("none")
        set_symbols('debug')
    else
        add_links("plugin")
        set_optimize("aggressive")
        set_symbols('hidden')
    end