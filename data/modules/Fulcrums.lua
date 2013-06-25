-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local loaded
local fulcrum
local playerarrived=false
local turrets={}
local turret_target=nil

local spawnTurrets = function (ship)
 	Timer:CallAt(Game.time+1, function ()
		if ship==nil or not ship:exists() then return end
		for x=1,5 do
			turrets[x]=Space.SpawnShipNear('military_gun_emplacement', fulcrum, 1, 1)
			turrets[x]:AddEquip('PULSECANNON_20MW')
			turrets[x]:AddEquip('PULSECANNON_20MW')
			turrets[x]:AddEquip('LASER_COOLING_BOOSTER')
			turrets[x]:SetLabel('[Military Sentry]')
			turrets[x]:AIHoldPos()
		end
	end)
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

local onEnterSystem = function (player)
	
	if player:IsPlayer() and spawnShips()~=nil then
		if fulcrum~=nil then
			local x,y,z = fulcrum:GetPos()
			y=y-200
			Game.player:SetPos(fulcrum,x,y,z)
			fulcrum:UseECM()
			Game.player:AIFlyToClose(fulcrum,500)
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
	if turrets~=nil and ship==turret_target then 
		for k,v in pairs(turrets) do
			if v==attacker then
				v:CancelAI()
				v:AIHoldPos()
			end
		end
		--turret_target = nil
	end
end

local onAICompleted = function (ship, ai_error)
	if not ship:IsPlayer() and playerarrived==true then return end
	playerarrived=false
end


local onGameStart = function ()
	if loaded == nil then
		spawnShips()
	end
	loaded = nil
end

local onShipHit = function (ship, attacker)
	if ship==nil or not ship:exists() or ship==attacker then return end
	if turrets==nil then return end
	for k,v in pairs(turrets) do
		if v~=nil and v:exists() and v:isa('Ship') and (ship==fulcrum or v==ship or ship==Game.player) then
			v:AIKill(attacker)
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
end

local onGameEnd = function ()
	-- drop the references for our data so Lua can free them
	-- and so we can start fresh if the player starts another game
	loaded,fulcrum,playerarrived,turrets=nil,nil,nil,nil
end

Event.Register("onShipDestroyed", onShipDestroyed)
Event.Register("onAICompleted", onAICompleted)
Event.Register("onShipHit", onShipHit)
Event.Register("onGameEnd", onGameEnd)
Event.Register("onEnterSystem", onEnterSystem)
Event.Register("onGameStart", onGameStart)
Serializer:Register("Fulcrum", serialize, unserialize)
