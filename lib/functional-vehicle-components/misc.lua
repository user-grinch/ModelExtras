local module = {}

function module.Rotator(veh, comp)

    local x_comp_name = futil.GetValue(nil,comp.name,"_x%((.*)%)")
    local y_comp_name = futil.GetValue(nil,comp.name,"_y%((.*)%)")
    local z_comp_name = futil.GetValue(nil,comp.name,"_z%((.*)%)")

    if x_comp_name == nil
    and y_comp_name == nil
    and z_comp_name == nil then
        flog.Write("No '_x' or '_y' or '_z' found in ".. comp.name)
        return
    end

    local current_val = 0
    local current_val2  = 0
    local current_val3 = 0
    local call_func = nil
    local matrix = comp.modeling_matrix
    local mul = futil.GetValue(50,comp.name,"_m(-?%d+)")

    if x_comp_name ~= nil then
        call_func = function()
            local export_val = futil.GetExport(x_comp_name,"rotx",current_val)*mul

            if current_val ~=  export_val then
                matrix:rotate_x(export_val)
                current_val =  export_val
            end
        end
    end

    if y_comp_name ~= nil then
        call_func = function()
            local export_val = futil.GetExport(y_comp_name,"roty",current_val)*mul
            
            if current_val ~=  export_val then
                matrix:rotate_y(export_val)
                current_val =  export_val
            end
        end
    end

    if z_comp_name ~= nil then
        call_func = function()
            local export_val = futil.GetExport(z_comp_name,"rotz",current_val)*mul
            
            if current_val ~=  export_val then
                matrix:rotate_z(export_val)
                current_val =  export_val
            end
        end
    end

    if call_func == nil then
        call_func = function()
            local xexport_val = futil.GetExport(x_comp_name,"rotx",current_val)*mul
            local yexport_val = futil.GetExport(y_comp_name,"roty",current_val)*mul
            local zexport_val = futil.GetExport(z_comp_name,"rotz",current_val)*mul

            if current_val ~=  xexport_val
            and current_val2 ~=  yexport_val 
            and current_val3 ~=  zexport_val then
                matrix:rotate(xexport_val,yexport_val,zexport_val)
                current_val =   xexport_val
                current_val2 =  yexport_val
                current_val3 =  zexport_val
            end
        end
    end

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end
        
        call_func()

        wait(0)
    end

end

function module.Gun(veh, comp)

    local pmatrix = comp.modeling_matrix

    local src_comp = nil
    local tar_comp = nil
    local cam_comp = nil

    for _, child_comp in ipairs(comp:get_child_components()) do
        if child_comp.name == "src" then
            src_comp = child_comp
        end
        if child_comp.name == "tar" then
            tar_comp = child_comp
        end
        if child_comp.name == "cam" then
            cam_comp = child_comp
        end
    end

    if cam_comp == nil or tar_comp == nil or src_comp == nil then
        flog.Write("Exiting, 'cam', 'tar' or 'src' dummy might be missing in " .. comp.name)
        return
    end

    local wait_time = futil.GetValue(120,comp.name,"_ms(%d+)")
    local damage = futil.GetValue(50,comp.name,"_d(%d+)")
    local mul = futil.GetValue(0.5,comp.name,"_m(%d+)")
    local limit_up = futil.FindChildData(comp, "rl_up=(%d+)",360)/120
    local limit_down = futil.FindChildData(comp, "rl_down=(%d+)",360)/120
    local limit_left = futil.FindChildData(comp, "rl_left=(%d+)",360)/120
    local limit_right = futil.FindChildData(comp, "rl_right=(%d+)",360)/120
    local key_name = futil.FindChildData(comp, "key=(.*)","default_gun_key")

    local total_x = 0
    local total_z = 0
    local last_shot_time = 0
    local factor = 285
    local reset_cam = false
    local current_wanted_heat = math.random(0,500)
    local heat_required_to_increase_lvl = 500

    futil.UpdateExport(key_name,"roty",0)

    function ReportCrime(fromX,fromY,fromZ,toX,toY,toZ)
        _,wl = storeWantedLevel(PLAYER_HANDLE)

        if wl == 6 then
            return
        end

        if isLineOfSightClear(fromX,fromY,fromZ,toX,toY,toZ,false,true,true,false,false) then
            current_wanted_heat = current_wanted_heat + 5
        else
            current_wanted_heat = current_wanted_heat + 30
        end

        wl = wl+1

        if current_wanted_heat > heat_required_to_increase_lvl*wl then
            current_wanted_heat = 0
            alterWantedLevel(PLAYER_HANDLE,wl)
        end
    end

    flog.ProcessingComponent(comp.name)

    while true do
        if futil.VehicleCheck(veh) then return end
        
        if isKeyDown(vkeys.VK_LSHIFT) and isCharInCar(PLAYER_PED,veh) then
            local mouseX, mouseY = getPcMouseMovement()
            mouseX = mouseX*mul
            mouseY = mouseY*mul
            local sx,sy,sz = src_comp.matrix.pos:get()
            local tx,ty,tz = tar_comp.matrix.pos:get()
            local cx,cy,cz = cam_comp.matrix.pos:get()

            reset_cam = true
            setFixedCameraPosition(cx,cy,cz,0,0,0)
            pointCameraAtPoint(tx,ty,tz,2)

            
            local screenX,screenY = convert3DCoordsToScreen(tx,ty,tz)
            screenX,screenY = convertWindowScreenCoordsToGameScreenCoords(screenX,screenY)
            drawSprite(tmain.txd["crosshair"],screenX,screenY,15,15,255,255,255,255)
            
            mouseX = mouseX/factor
            mouseY = mouseY/factor

            if mouseX > 0 and  limit_right ~= 0 then -- moving right
                if limit_right ~= 3 and  (total_z+mouseX) > limit_right then
                    total_z = limit_right
                else
                    total_z = total_z + mouseX
                end
            else
                if mouseX < 0 and limit_left ~= 0 then -- moving left 
                    if limit_left ~= 3 and  (total_z+mouseX) < limit_left*-1 then
                        total_z = limit_left*-1
                    else
                        total_z = total_z + mouseX
                    end               
                end
            end

            if mouseY > 0 and limit_up ~= 0 then -- moving up
                if limit_up ~= 3 and  (total_x+mouseY) > limit_up then
                    total_x = limit_up
                else
                    total_x = total_x + mouseY
                end
            else
                if mouseY < 0 and limit_down ~= 0 then -- moving down 
                    if limit_down ~= 3 and  (total_x+mouseY) < limit_down*-1 then
                        total_x = limit_down*-1
                    else
                        total_x = total_x + mouseY
                    end               
                end
            end	
                                                    
            pmatrix:rotate(total_x,0,total_z*-1)
            futil.UpdateExport(key_name,"rotx",total_x)
            futil.UpdateExport(key_name,"rotz",total_z*-1)

            if (isKeyDown(vkeys.VK_LCONTROL) or isKeyDown(vkeys.VK_LBUTTON)) 
            and (getGameTimer()-last_shot_time) > wait_time then

                last_shot_time = getGameTimer()

                fireSingleBullet(sx,sy,sz,tx,ty,tz,damage)
                ReportCrime(sx,sy,sz,tx,ty,tz)
                reportMissionAudioEventAtCar(veh,1157)
                
                addBigGunFlash(sx,sy,sz,tx,ty,tz)
            
            end
        else
            if reset_cam == true then
                restoreCameraJumpcut()
                reset_cam = false
            end
        end
        wait(0)
    end
end

function module.Chain(veh,comp)

    local speed = nil
    local chain_table = {}
    local time = futil.GetValue(5,comp.name,"_ms(%d+)")

    for _, comp in ipairs(comp:get_child_components()) do
        table.insert(chain_table, comp)
    end

    rotate_chain = function(i)
        if futil.VehicleCheck(veh) then return end
        speed = futil.GetRealisticSpeed(veh, 1)

        if speed >= 2 or speed <= -2 then
            futil.HideChildsExcept(chain_table,i)
            wait(time / math.abs(speed))
        end
    end

    flog.ProcessingComponent(comp.name)
    while true do

        if futil.VehicleCheck(veh) then return end

        speed = futil.GetRealisticSpeed(veh, 1)
        
        if speed >= 2 then
            for i = 1, #chain_table, 1 do
                rotate_chain(i)
            end
        end
        if speed <= -2 then
            for i = #chain_table, 1, -1 do
                rotate_chain(i)
            end
        end
        
       
        wait(0)
    end
end

function module.ExtraWheels(veh,comp)
    
    local wheel_no = futil.GetValue(1,comp.name,"_w(%d+)")
    local rot_mul = futil.GetValue(30,comp.name,"_rmul(%d+)")

    local pVeh = getCarPointer(veh)
    local model = getCarModel(veh)
    local matrix = comp.modeling_matrix
    local rotation = 0
    local fWheelSpeed = nil
    
    if isThisModelABike(model) then
        fWheelSpeed = ffi.cast("float*", pVeh + 0x758)
        wheel_no = wheel_no > 1 and 1 or wheel_no
    end
    if isThisModelACar(model) then
        fWheelSpeed = ffi.cast("float*", pVeh + 0x848)
        wheel_no = wheel_no > 3 and 3 or wheel_no
    end

    if fWheelSpeed == nil then   
        flog.Write(string.format("%s can't have extra wheel.",futil.GetNameOfVehicleModel(model)))
        return
    end

    flog.ProcessingComponent(comp.name)
    while true do

        if futil.VehicleCheck(veh) then return end
        
        rotation = rotation + fWheelSpeed[wheel_no]*rot_mul

        if rotation > 360 then
            rotation = rotation - 360
        end
        if rotation < 0 then
            rotation = 360 + rotation
        end
        matrix:rotate_x(rotation)

        wait(0)
    end
end

function module.GearLever(veh, comp)

    local normal_angle = futil.GetValue(15,comp.name,"_ax(%w+)")
    local offset_angle = futil.GetValue(15,comp.name,"_o(%w+)")
    local wait_time    = futil.GetValue(1,comp.name,"_ms(%d+)")
    local gear_type    = futil.GetValue(2,comp.name,"_t(%d+)")

    local matrix = comp.modeling_matrix
    local current_gear = -1

    rotate_gear = function(val)
        local change_angle = normal_angle - offset_angle*val

        for i = normal_angle, change_angle, 3*val do
            matrix:rotate_x(i)
            wait(wait_time)
        end
        for i = change_angle, normal_angle, 3*val do
            matrix:rotate_x(i)
            wait(wait_time)
        end
    end

    flog.ProcessingComponent(comp.name)
    while true do

        if futil.VehicleCheck(veh) then return end
       
        local gear = getCarCurrentGear(veh)
        if gear ~= current_gear then
            local val = 1

            if current_gear > gear then
                val = -1
            end

            if gear_type == 1 then
                rotate_gear(val)
            else
                -- N-> 1
                if current_gear == 0 then
                    rotate_gear(1)
                end

                -- 1->4
                if current_gear > 0 and val == 1 then
                    rotate_gear(-1)
                end

                -- 4->1
                if current_gear > 0 and val == -1 then
                    rotate_gear(1)
                end

                -- 1->N
                if current_gear == 1 and val == -1 then
                    local temp = offset_angle
                    offset_angle = offset_angle/2
                    rotate_gear(-1)
                    offset_angle = temp
                end
            end

            current_gear = gear
        end

        wait(0)
    end
end

function module.Clutch(veh, comp)

    local rotation_angle = futil.GetValue(17,comp.name,"_az(%w+)")
    local wait_time = futil.GetValue(1,comp.name,"_ms(%d+)")

    local matrix = comp.modeling_matrix
    local current_gear = 0

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end

        if getCarCurrentGear(veh) ~= current_gear then
            current_gear = getCarCurrentGear(veh)
            for i = 0, rotation_angle / 4, 1 do
                matrix:rotate_z(i * 4)
                wait(wait_time)
            end
            for i = rotation_angle / 4, 0, -1 do
                matrix:rotate_z(i * 4)
                wait(wait_time)
            end
        end
        wait(0)
    end
end

function module.Throttle(veh, comp)

    local rotate_axis = futil.GetValue("x",comp.name,"_(%w)=")
    local rotation = futil.GetValue(50,comp.name,"=(-?%d[%d.]*)")
    local wait_time = futil.GetValue(50,comp.name,"_ms(%d+)")
    
    local rotx, roty, rotz

    rotation = rotation/20 -- manual test
    if rotate_axis ~= "x" then
        rotx = futil.FindChildData(comp, "rotx=(-?%d[%d.]*)",50)
    else
        rotx = rotation
    end
    if rotate_axis ~= "y" then
        roty = futil.FindChildData(comp, "roty=(-?%d[%d.]*)",0)
    else
        roty = rotation
    end
    if rotate_axis ~= "z" then
        rotz = futil.FindChildData(comp, "rotz=(-?%d[%d.]*)",0)
    else
        rotz = rotation
    end

    local matrix = comp.modeling_matrix
    local current_state = 0

    flog.ProcessingComponent(comp.name)

    local rotate_throttle = function(i,cur_rotx,cur_roty,cur_rotz)
        if rotate_axis == "x" then
            cur_rotx = cur_rotx + i
        end
        if rotate_axis == "y" then
            cur_roty = cur_roty + i
        end
        if rotate_axis == "z" then
            cur_rotz = cur_rotz + i
        end

        matrix:rotate(cur_rotx,cur_roty,cur_rotz)
        wait(wait_time)
    end

    while true do
        if futil.VehicleCheck(veh) then return end

        local fGas_state = math.floor(memory.getfloat(getCarPointer(veh)+0x49C))
        local gear = getCarCurrentGear(veh)

        if fGas_state ~= current_state then -- fGas_state
            current_state = fGas_state

            if gear >= 1 then
                local cur_rotx = rotx or rotation
                local cur_roty = roty or rotation
                local cur_rotz = rotz or rotation

                if futil.VehicleCheck(veh) then return end

                if fGas_state == 1 then      
                    for i=0,rotation,0.5 do
                        rotate_throttle(i,cur_rotx,cur_roty,cur_rotz)
                    end
                else
                    for i=rotation,0,-0.5 do
                        rotate_throttle(i,cur_rotx,cur_roty,cur_rotz)
                    end
                end

                while doesVehicleExist(veh) and math.floor(memory.getfloat(getCarPointer(veh)+0x49C)) == fGas_state do
                    wait(0)
                end
            end
        end
        wait(0)
    end
end

function module.FrontBrake(veh, comp)

    local offset_angle = futil.GetValue(15,comp.name,"_az(-?%d[%d.]*)")
    local wait_time = futil.GetValue(1,comp.name,"_ms(%d)")

    local matrix = comp.modeling_matrix
    local pveh = getCarPointer(veh)

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end

        if memory.getfloat(pveh + 0x4A0) == 1 then -- bIsHandbrakeOn

            local temp = 1

            if offset_angle < 0 then temp = -1 end

            for i = 0, offset_angle / 4, temp do
                matrix:rotate_z(i * 4)
                wait(wait_time)
            end

            while memory.getfloat(pveh + 0x4A0) == 1 do wait(0) end

            for i = offset_angle / 4, 0, (temp * -1) do
                matrix:rotate_z(i * 4)
                wait(wait_time)
            end

        end

        wait(0)
    end
end

function module.RearBrake(veh, comp)

    local offset_angle = futil.GetValue(5,comp.name,"_ax(-?%d[%d.]*)")
    local wait_time = futil.GetValue(1,comp.name,"_ms(%d)")

    local matrix = comp.modeling_matrix
    local pveh = getCarPointer(veh)

    flog.ProcessingComponent(comp.name)
    while true do
        if futil.VehicleCheck(veh) then return end

        if bit.band(memory.read(pveh + 0x428, 2), 32) == 32 then -- m_fBrakePedal

            local temp = 1

            if offset_angle < 0 then temp = -1 end
            
            for i = 0, offset_angle/2, temp do
                matrix:rotate_x(i*2)
                wait(wait_time)
            end

            while bit.band(memory.read(pveh + 0x428, 2), 32) == 32 do
                wait(0)
            end

            for i = offset_angle/2, 0, (temp * -1) do
                matrix:rotate_x(i*2)
                wait(wait_time)
            end

        end

        wait(0)
    end
end


return module