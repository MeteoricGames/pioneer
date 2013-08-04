-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local loaded
local fulcrum
local playerarrived=false
local turrets={}
local turret_target=nil
local lastCheck=0
local target

local check = function(ship)
	if ship~=nil and ship:exists() and ship:isa('Ship') then return true end
	return false
end

local spawnTurrets = function (ship)
 	Timer:CallAt(Game.time+1, function ()
		if turrets==nil then turrets={} end
		if ship==nil or not ship:exists() then return end
		for x=1,5 do
			local t	=Space.SpawnShipNear('military_gun_emplacement', fulcrum, 1, 1)
			turrets[t] = {
				ship	= t,
				status	='idle',
				killwho = nil,
			}
			turrets[t].ship:AddEquip('PULSECANNON_20MW')
			turrets[t].ship:AddEquip('PULSECANNON_20MW')
			turrets[t].ship:AddEquip('LASER_COOLING_BOOSTER')
			turrets[t].ship:SetLabel('[Military Sentry]')
			turrets[t].ship:AIHoldPos()
		end
	end)
end

local checkTurrets = function (time)
	if time-lastCheck > 2 then
		lastCheck=time
	else
		return
	end

		

		if turrets==nil then return end
		for k,v in pairs(turrets) do
			if check(target) and target == Game.player then target=Game.player end -- update pos.
			-- inrange
			     if check(v.ship) and v.status == 'idle' and check(target) and v.ship:DistanceTo(target)<4000 then
				v.status = 'cancel'
				v.ship:CancelAI()
				print "have idle, inrange, set cancel"
			elseif check(v.ship) and v.status == 'cancel' and check(target) and v.ship:DistanceTo(target)<4000 then
				v.status = 'attack'
				v.ship:AIKill(target)
				print "have cancel, inrange, set kill/attack"
			-- out of range
			elseif check(v.ship) and v.status == 'attack' and check(target) and v.ship:DistanceTo(target)>=4000 then
				v.status = 'cancel'
				v.ship:CancelAI()
				print "have attack, out of range, set cancel"
			elseif check(v.ship) and v.status == 'cancel' and check(target) and v.ship:DistanceTo(target)>=4000 then
				v.status = 'idle'
				v.ship:AIHoldPos()
				print "have cancel, out of range, setidle, and hold"
			-- target is dead
			elseif check(v.ship) and not check(target) and not v.status == 'cancel' then
				v.status = 'cancel'
				v.ship:CancelAI()
				print "either, cancel"
			elseif check(v.ship) and not check(target) and v.status == 'cancel' then
				v.status = 'idle'
				v.ship:AIHoldPos()
				print "have cancel either, set idle and hold"
			else
				print ('nothing.. stus :'..v.status)	
				if check(target) and check(v.ship) then print(' dist:'..v.ship:DistanceTo(target)) end
			end
		end
end

local spawnShips = function ()
	local population = Game.system.population

	if population == 0 then
		return	
	end

	local stations = Space.GetBodies(function (body) 
		return body:isa("SpaceStation") and body.type == 'STARPORT_SURFACE'
	end)
	if #stations < 2 then
		fulcrum=nil
		playerarrived=true
		return
	end

	local station = stations[1]
	fulcrum = Space.SpawnShipParkedOffset('large_fulcrum', station)
	fulcrum:SetLabel('[--Fulcrum--]')
	fulcrum:AddEquip("ECM_ADVANCED")

	playerarrived=true

	spawnTurrets(fulcrum)
	
	return 0
end

local onLeaveSystem = function (player)
	turrets=nil
	turrets={}
end

local onEnterSystem = function (player)
	
	if player:IsPlayer() and spawnShips()~=nil then
		if fulcrum~=nil then
			local x,y,z = fulcrum:GetPos()
			y=y-200
			Game.player:SetPos(fulcrum,x,y,z)
			fulcrum:UseECM()
			Game.player:AIFlyToClose(fulcrum,500)
			checkTurrets(Game.time)
		end
	else
		if fulcrum~=nil and player:exists() and fulcrum:exists() then
			local x,y,z = fulcrum:GetPos()
			y=y+200
			player:SetPos(fulcrum,x,y,z)
			fulcrum:UseECM()
		end
	end
end

local onShipDestroyed = function (ship, attacker)
	if turrets~=nil then checkTurrets(Game.time) end
end

local onAICompleted = function (ship, ai_error)

	if turrets~=nil then checkTurrets(Game.time) end
	if ship==nil then return end
	if not ship:exists() then return end

	if not ship:IsPlayer() and playerarrived==true then return end
	playerarrived=false
end

local onShipAlertChanged = function (ship, alert)
	if turrets==nil then return end
	checkTurrets(Game.time)
end

local onGameStart = function ()
	if loaded == nil then
		spawnShips()
		checkTurrets(Game.time)
	end
	loaded = nil
end

local onShipHit = function (ship, attacker) --attacker location is a snapshot..bah..
	if ship==nil or not ship:exists() or ship==attacker or not ship==fulcrum then return end
	if turrets==nil then return end
	for k,v in pairs(turrets) do
		if check(attacker) and check(v.ship) then
			target=attacker
			checkTurrets(Game.time)
		end
	end
end

local serialize = function ()
	return {playerarrived,fulcrum}
end

local unserialize = function (data)
	loaded = true
	playerarrived=data[1]
	fulcrum=data[2]
	spawnTurrets(fulcrum)
	checkTurrets(Game.time)
end

local onGameEnd = function ()
	-- drop the references for our data so Lua can free them
	-- and so we can start fresh if the player starts another game
	loaded,fulcrum,playerarrived,turrets=nil,nil,nil,nil
end

--Event.Register("onShipDestroyed", onShipDestroyed)
Event.Register("onAICompleted", onAICompleted)
Event.Register("onShipHit", onShipHit)
Event.Register("onGameEnd", onGameEnd)
Event.Register("onEnterSystem", onEnterSystem)
Event.Register("onLeaveSystem", onLeaveSystem)
Event.Register("onGameStart", onGameStart)
Event.Register("onShipAlertChanged", onShipAlertChanged)
Serializer:Register("Fulcrum", serialize, unserialize)
