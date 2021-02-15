
function UpdateOdometerNumber(number,angle,child_comps,shown_angle,default_type)

    if number > 999999 then number = 999999 end

    local angle_table = {}

    for c in string.gmatch(tostring(number), ".") do
        table.insert(angle_table, 1, tonumber(c) * angle)
    end
    
    for i = 1, 6, 1 do
        if child_comps[i] ~= nil then

            if angle_table[i] == nil then angle_table[i] = 0 end
            if shown_angle[i] == nil then shown_angle[i] = -1 end

            local matrix = child_comps[i].modeling_matrix

            while shown_angle[i] ~= angle_table[i] do

                if shown_angle[i] ~= -1 then
                    if shown_angle[i] == angle * 9 then

                        while shown_angle[i] ~= angle * 10 do
                            shown_angle[i] = shown_angle[i] + angle / 16
                            matrix:rotate_x(shown_angle[i])

                            if default_type ~= "digital" then
                                wait(25)
                            end
                        end

                        shown_angle[i] = angle_table[i]
                        goto breakloops
                    end

                    if shown_angle[i] > angle_table[i] then
                        shown_angle[i] = shown_angle[i] - angle / 16
                    end
                    if shown_angle[i] < angle_table[i] then
                        shown_angle[i] = shown_angle[i] + angle / 16
                    end
                    matrix:rotate_x(shown_angle[i])
                    
                    if default_type == "analog" then
                        wait(25)
                    end
                else
                    matrix:rotate_x(angle_table[i])
                    shown_angle[i] = angle_table[i]
                end
            end
        end
        ::breakloops::
    end

end

function module.Odometer(veh,comp)

    local rotation_angle = futil.GetValue(36,comp.name,"_ax(%w+)")
    local default_type = futil.GetValue("analog",comp.name,"_t(%w+)")
    local unit = futil.FindChildData(comp,"unit=","mph")
    
    local current_number = 0
    local new_number = fgsx.Get(veh,"odo_val") or 0
    local bac = 0
    local offset = nil
    local model = getCarModel(veh)
    local shown_angle = {}
    local child_comps = {}
    local mul = 160.9

    if unit == "kph" then
        mul = 100
    end

    for index, child in ipairs(comp:get_child_components()) do
        table.insert(child_comps, child)
    end

    if isThisModelABike(model) then
        offset = 0x750
    else
        if isThisModelACar(model) then 
            offset = 0x828 
        end
    end

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) or offset == nil then return end

        local val = math.abs(math.floor(memory.getfloat(getCarPointer(veh) + offset) / (2.86*mul)))
        new_number = new_number + math.abs(bac - val)
        bac = val
        if current_number ~= new_number then
            current_number = new_number
            UpdateOdometerNumber(current_number,rotation_angle,child_comps,shown_angle,default_type)
        end
        wait(0)
    end
end

function module.Speedometer(veh,comp)

    local angle_start = futil.GetValue(0,comp.name,"_ay(-?%d[%d.]*)")
    local angle_end = futil.GetValue(180,comp.name,"_(-?%d[%d.]*)")

    local matrix = comp.modeling_matrix
    local unit = futil.FindChildData(comp,"unit=","mph")
    local speedm_max = futil.GetValue(180,comp.name,"_m(%d+)")

    local total_rot = futil.CalcTotalRotation(angle_start,angle_end)
    local temp = 0
    local rotation = 0
    
    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end

        local speed = math.abs(futil.GetRealisticSpeed(veh))
        
        
        if unit == "mph" then speed = speed / 1.6 end

        if speed > speedm_max then speed = speedm_max end
        
        temp = total_rot / speedm_max * speed + angle_start

        if rotation < temp then 
            rotation = rotation + (temp-rotation)/5
        end

        if rotation > temp then 
            rotation = rotation - (rotation-temp)/5
        end
        
        matrix:rotate_y(rotation)
        
        wait(0)
    end
end

function module.RPMmeter(veh, comp)

    local angle_start = futil.GetValue(0,comp.name,"_ay(-?%d+)")
    local angle_end = futil.GetValue(180,comp.name,"_(-?%d+)")
    local matrix = comp.modeling_matrix

    flog.ProcessingComponent(comp.name)

    local meter_max = futil.GetValue(9,comp.name,"_m(%d+)")
    local total_rot = math.abs(angle_end) + math.abs(angle_start)
    local cur_rpm = 0
    local temp = 0
    local rotation = 0
    local cur_gear = 0

    while true do
        if futil.VehicleCheck(veh) then return end

        local rea_speed = futil.GetRealisticSpeed(veh)

        if math.floor(memory.getfloat(getCarPointer(veh)+0x49C)) ~= 0 then
            cur_gear = getCarCurrentGear(veh)
        end
        
        if cur_gear ~= 0 then
            cur_rpm = rea_speed/cur_gear - 2
        end

        cur_rpm = cur_rpm < 0 and 0 or cur_rpm
        cur_rpm = cur_rpm > meter_max and meter_max or cur_rpm

        temp = total_rot / meter_max * cur_rpm + angle_start
        
        if isCarEngineOn(veh) then
           temp = temp + 20
        end

        if rotation < temp then 
            rotation = rotation + (temp-rotation)/3
            rotation = rotation > angle_end and angle_end or rotation
        end

        if rotation > temp then 
            rotation = rotation - (rotation-temp)/3
            rotation = rotation < angle_start and angle_start or rotation
        end
        
        matrix:rotate_y(rotation)

        wait(0)
    end
end

function module.DigitalGearMeter(veh,comp)

    local number_table = {}
    
    for _, comp in ipairs(comp:get_child_components()) do
        table.insert(number_table, comp)
    end

    flog.ProcessingComponent(comp.name)
    while true do

        if futil.VehicleCheck(veh) then return end
        futil.HideChildsExcept(number_table,getCarCurrentGear(veh)+1)
        
        wait(0)
    end
end


function module.FuelMeter(veh,comp)

    local angle_start = futil.GetValue(35,comp.name,"_ay(-?%d[%d.]*)")
    local angle_end = futil.GetValue(155,comp.name,"_(-?%d[%d.]*)")

    local matrix = comp.modeling_matrix

    local total_rot = futil.CalcTotalRotation(angle_start,angle_end)
    local rotation = 0

    fgsx.Set(veh,"fm_fuel",math.random(30, 90))
    flog.ProcessingComponent(comp.name)
    
    while true do
        if futil.VehicleCheck(veh) then return end

        local fuel = fgsx.Get(veh,"fm_fuel") or 0
        local speed = futil.GetRealisticSpeed(veh)
        rotation = (total_rot / 100) * fuel + angle_start
        
        matrix:rotate_y(rotation > angle_end and angle_end or rotation)

        if isCarEngineOn(veh) then
            fuel = fuel > 1 and fuel - speed/100000 or fuel
            fgsx.Set(veh,"fm_fuel",fuel)
        end

        wait(0)
    end
end

function DrawFuelBar(fuel)
    if fuel < -24 then fuel = -24 end
    local resX, resY = getScreenResolution()
    local posX = resX/ 1.17
    local posY = resY / 4.3
    local ratioX = resX/1366
    local ratioY = resY/768
    
    if readMemory(0xA444A0,1,false) == 1 then -- hud enabled
        mad.draw_rect(posX,posY,posX+130*ratioX,posY+15*ratioY,0,0,0,255,0.0)
        mad.draw_rect(posX+3.5*ratioX,posY+3.5*ratioY,posX+126.5*ratioX,posY+11.5*ratioY,128,128,128,255,0.0)
        mad.draw_rect(posX+3.5*ratioX,posY+3.5*ratioY,posX+(26.5+fuel)*ratioX,posY+11.5*ratioY,148,146,145,255,0.0)
    end
end

function module.IsPlayerNearFuel()
    while true do
        if isCharInAnyCar(PLAYER_PED) then
            local veh = getCarCharIsUsing(PLAYER_PED)
            local x,y,z = getCarCoordinates(veh)
            for _,obj in ipairs(mad.get_all_objects(x,y,z,5.0,false)) do
                local model =  getObjectModel(obj)
                local fuel = fgsx.Get(veh,"fm_fuel") or 0
                if (model == 1676 or model == 1686 or model == 3465) and fuel < 80 then
                    printString("Press Space to refuel your vehicle",100)
                    if isKeyDown(0x20) then
                        fgsx.Set(veh,"fm_fuel",100)
                        printString("Vehicle refueled",1000)
                        wait(1000)
                    end
                end
            end
            DrawFuelBar(fuel)
        end
        
        wait(0)
    end
end