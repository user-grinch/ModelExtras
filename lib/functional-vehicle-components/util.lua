local module = {}

function module.VehicleCheck(veh)
    if not doesVehicleExist(veh) or isCarDead(veh) then 
        return true
    else
        return false
    end
end

function module.GetSkygfxVehpipeColor(normal_color, special_color)
    if tmain.skygfx.pconfig ~= nil then
        local veh_pipe = readMemory(tmain.skygfx.pconfig+56,4,false)

        if (veh_pipe == 1 or veh_pipe == 2) and special_color ~= nil then -- PC & XBOX pipeline
            return table.unpack(special_color)
        end
    end
    return table.unpack(normal_color)
end

function FindComponentData(comp_name,data_pattern)
    local comp_name = comp_name:gsub(",",".")
    local _,_,data = string.find(comp_name, data_pattern)

    if data then
        return data
    end
    return nil
end

function module.GetNameOfVehicleModel(model)
    return ffi.string(ffi.cast("char*",CVehicleModelInfo[tonumber(model)] + 0x32)) or ""
end

function module.CalcTotalRotation(angle_start,angle_end)
    if (angle_start > 0 and angle_end > 0) or (angle_start < 0 and angle_end < 0) then
        return math.abs(angle_end) - math.abs(angle_start)
    else
        return math.abs(angle_end) + math.abs(angle_start)
    end
end

function module.GetValue(default_value,comp_name,model_data_prefix)
    local rtn_val = FindComponentData(comp_name,model_data_prefix)

    if type(default_value) == "number" then
        rtn_val = tonumber(rtn_val)
    end
    
    return rtn_val or default_value
end
    
-- This function is taken from juniors vehfuncs
function module.GetRealisticSpeed(veh, wheel)

    if module.VehicleCheck(veh) then return end
    
    local model = getCarModel(veh)
    local pVeh = getCarPointer(veh)
    local realisticSpeed = 0
    local frontWheelSize = memory.getfloat(
                               CVehicleModelInfo[getCarModel(veh)] + 0x40)
    local rearWheelSize = memory.getfloat(
                              CVehicleModelInfo[getCarModel(veh)] + 0x44)

    if isThisModelABike(model) then -- bike
        local fWheelSpeed = ffi.cast("float*", pVeh + 0x758) -- CBike.fWheelSpeed[]

        if wheel == nil then
            realisticSpeed = (fWheelSpeed[0] * frontWheelSize + 0.08 + fWheelSpeed[1] * rearWheelSize) / 2
        else
            if wheel == 0 then
                realisticSpeed = fWheelSpeed[wheel] * frontWheelSize + 0.08
            else
                realisticSpeed = fWheelSpeed[wheel] * rearWheelSize
            end
        end
    end

    if isThisModelAPlane(model) or isThisModelAHeli(model) or isThisModelABoat(model) then
        realisticSpeed = getCarSpeed(veh) * -0.0426 -- Manually tested
    else
        if isThisModelACar(model) then -- car
            local fWheelSpeed = ffi.cast("float*", pVeh + 0x848) -- Automobile.fWheelSpeed[]
            if wheel == nil then
                realisticSpeed = (fWheelSpeed[0] * frontWheelSize + fWheelSpeed[1] * frontWheelSize + fWheelSpeed[2] * rearWheelSize +
                                    fWheelSpeed[3] * rearWheelSize) / 4
            else
                if wheel == 0 or wheel == 1 then
                    realisticSpeed = fWheelSpeed[wheel] * frontWheelSize
                else
                    realisticSpeed = fWheelSpeed[wheel] * rearWheelSize
                end
            end   
        end
    end

    realisticSpeed = realisticSpeed / 2.45 -- tweak based on distance (manually testing)
    realisticSpeed = realisticSpeed * -186.0 -- tweak based on km/h

    return realisticSpeed
end

function module.FindChildData(parent_comp,data_prefix,default_value)

    local data 
    for _, child_comp in ipairs(parent_comp:get_child_components()) do
        child_comp_name = child_comp.name:gsub(",",".")
        _,_,data = child_comp_name:find(data_prefix)
        if data then
            return data
        end
    end
    return default_value
end

function module.UpdateExport(name,key,val)

    if tmain.exports[name] == nil then tmain.exports[name] = {} end
    tmain.exports[name][key] = val
end

function module.GetExport(comp_name,key,default)

    if tmain.exports[comp_name] == nil then return default end
    
    return tmain.exports[comp_name][key] or default
end

function module.GetChilds(parent_comp,data_prefix,tbl)
    
    for _, child_comp in ipairs(parent_comp:get_child_components()) do
        if child_comp.name:find(data_prefix) then
            table.insert(tbl,child_comp)
        end
    end
end

function module.FindAnimations(parent_comp)
    local data = {}

    for _, child_comp in ipairs(parent_comp:get_child_components()) do
        if child_comp.name == "fc_anim" then
            for _, child_child_comp in ipairs(child_comp:get_child_components()) do

                local file, anim = string.match(child_child_comp.name,
                                                "([^_]+)_([^_]+)")
                requestAnimation(file)
                loadAllModelsNow()
                table.insert(data, file)
                table.insert(data, anim)
                flog.Write(string.format("Found animation '%s'",anim))
            end
            return table.unpack(data)
        end
    end
    return {}
end

function module.SetMaterialColor(comp,r,g,b,a)
    for _, obj in ipairs(comp:get_objects()) do
        for _, mat in ipairs(obj:get_materials()) do
            mat:set_color(r,g,b,a)
        end
    end
end

function module.HideChildsExcept(model_table,show_index)
    for j = 1, #model_table, 1 do
        if model_table[show_index].name == model_table[j].name then
            model_table[j]:set_alpha(255)
        else
            model_table[j]:set_alpha(0)
        end
    end
end

function module.HighlightComponent(veh, prefix)
    while true do
        for i, comp in ipairs(mad.get_all_vehicle_components(veh)) do
            if string.find(comp.name, prefix) ~= nil then
                for _, obj in ipairs(comp:get_objects()) do
                    for i, mat in ipairs(obj:get_materials()) do
                        mat:set_color(0, 255, 0, 255)
                    end
                end
                local x, y, z = comp.matrix.pos:get()
                local sx, sy = convert3DCoordsToScreen(x, y, z)
                mad.draw_text(string.format('%s', comp.name), sx, sy,
                              mad.font_style.SUBTITLES, 0.3, 0.6,
                              mad.font_align.LEFT, 2000, true, false, 255, 255,
                              255, 255, 1, 0, 30, 30, 30, 120)
            end
        end
        wait(0)
    end
end

return module