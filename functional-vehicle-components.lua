script_name('Functional Vehicle Components')
script_author("Grinch_")
script_version("1.0")
script_version_number(2020062801) -- YYYYMMDDNN
script_description("Adds more features/ functions to vehicle components")
script_dependencies("ffi", "Memory", "MoonAdditions", "log")
script_properties('work-in-pause')
script_url("https://github.com/user-grinch/Functional-Vehicle-Components")

--------------------------------------------------
-- Special thanks to kkjj & Zeneric for their help
--------------------------------------------------
WRITE_INFO_TO_LOG = true -- set it to false for no log

tmain = 
{
    dir = getWorkingDirectory() .. "/lib/functional-vehicle-components/",
    exports = {},
    gsx = {
        handle = getModuleHandle("gsx.asi"),
        veh_data = {},
    },
    skygfx = {
        handle  =  getModuleHandle("skygfx.asi"),
        pconfig = nil, 
    },
    txd = {}
}

if loadTextureDictionary(getWorkingDirectory() .. "/lib/functional-vehicle-components/textures") then
    tmain.txd["crosshair"] = loadSprite("crosshair")
end

--------------------------------------------------
-- Libraries & modules

ffi         = require 'ffi'
memory      = require 'memory'
mad         = require 'MoonAdditions'
vkeys       = require 'vkeys'

flog        = require 'lib.functional-vehicle-components.log'
futil       = require 'lib.functional-vehicle-components.util'

fdashboard  = require 'lib.functional-vehicle-components.dashboard'
fgsx        = require 'lib.functional-vehicle-components.gsx'
fmisc       = require 'lib.functional-vehicle-components.misc'
----------------------------------------------------

flog.Start()
math.randomseed(os.time())
CVehicleModelInfo = ffi.cast("uintptr_t*", 0xA9B0C8)
isThisModelABike = ffi.cast("bool(*)(int model)", 0x4C5B60)
fTimeStep = ffi.cast("float*",0xB7CB5C) -- CTimer::ms_fTimeStep

-- Get skygfx pconfig
if tmain.skygfx.handle ~= 0 then
    local result,addr =  getDynamicLibraryProcedure("GetConfig",tmain.skygfx.handle)

    if result then
        flog.Write("SkyGFX installed")
        tmain.skygfx.pconfig = callFunction(addr,0,0)
    end
end

local find_callback  = 
{
    ["fc_rot"]       = fmisc.Rotator,

    ["fc_chain"]     = fmisc.Chain,
    ["fc_cl"]        = fmisc.Clutch,
    ["fc_dled"]      = fdashboard.DamageLed,
    ["fc_fbrake"]    = fmisc.FrontBrake,
    ["fc_fm"]        = fdashboard.FuelMeter,
    ["fc_gl"]        = fmisc.GearLever,
    ["fc_gm"]        = fdashboard.DigitalGearMeter,
    ["fc_gun"]       = fmisc.Gun,
    ["fc_nled"]      = fdashboard.NeutralLed,
    ["fc_pled"]      = fdashboard.PowerLed,
    ["fc_om"]        = fdashboard.Odometer,
    ["fc_sm"]        = fdashboard.Speedometer,
    ["fc_th"]        = fmisc.Throttle,
    ["fc_rbrake"]    = fmisc.RearBrake,
    ["fc_rpm"]       = fdashboard.RPMmeter,
    ["fc_xwheel"]    = fmisc.ExtraWheels,
}

function main()

    local veh_data = tmain.gsx.veh_data
    -- result,handle = loadDynamicLibrary("VehFuncs.asi")
    -- result, proc = getDynamicLibraryProcedure("Ext_GetVehicleSpeedRealistic",handle)
    while true do
    
        for _, veh in ipairs(getAllVehicles()) do

            if doesVehicleExist(veh) and veh_data[veh] == nil then
                local model = getCarModel(veh)
                
                if tmain.gsx.handle ~= 0 then 
                    local pveh = getCarPointer(veh)
                    if DataToLoadExists(pveh,"FVC_DATA") == 1 then
                        local pdata = GetLoadDataByVehPtr(pveh,"FVC_DATA")
                        local size  = GetDataToLoadSize(pveh,"FVC_DATA") 
                        veh_data[veh] = decodeJson(memory.tostring(pdata,size,false))                 
                    end
                end

                if veh_data[veh] == nil then
                    fgsx.Set(veh,"odo_val",math.random(10000, 200000))
                end

                flog.Write("")
                flog.Write(string.format("Found vehicle %s (%d)",futil.GetNameOfVehicleModel(model), model))

                for _, comp in ipairs(mad.get_all_vehicle_components(veh)) do
                    local comp_name = comp.name
                    for name,func in pairs(find_callback) do
                        if string.find(comp_name,name) then
                            flog.Write(string.format("Found '%s' in '%s'",name,comp_name))
                            lua_thread.create(func,veh,comp)
                            break
                        end
                    end
                end
            end
            
        end
        -- if isCharInAnyCar(PLAYER_PED) then
        --     local veh = getCarCharIsUsing(PLAYER_PED)
        --     local pVeh = getCarPointer(veh)
        --     returnValue = callFunction(proc,1,1,pVeh)
        
        --     printString(string.format("%.3f",returnValue),100)
        -- end

        wait(0)
    end
    wait(0)
end

function onScriptTerminate(script, quitGame)
    if script == thisScript() then 
        flog.Close()
        if tmain.gsx.handle ~= 0 then
            fgsx.RemoveNotifyCallback(fgsx.pNotifyCallback)
        end
    end
end
