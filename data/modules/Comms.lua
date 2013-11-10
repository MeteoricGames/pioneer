-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.tx

local Engine = import("Engine")
local Game = import("Game")
local Space = import("Space")
local Comms = import("Comms")
local Timer = import("Timer")
local Event = import("Event")
local Serializer = import("Serializer")
local ShipDef = import("ShipDef")
local utils = import("utils")
local EquipDef = import("EquipDef")

local aship
local timelimit=0
local playerhaled=false

local 	NO_COMM_OR_REJECT	=0;
local	BEING_HALED		=1;
local 	OPEN_COMM		=2;
local 	ACCEPTED_COMM		=3;

local 	AGREED			=20;
local 	HALF_CARGO		=21;
local 	ALL_CARGO		=22;
local 	NO_DESTROY		=23;
local 	NOTIFY_POLICE		=24;

local 	HALE_A_SHIP		=200;
local 	BROADCAST_ATTACK	=100;
local 	BROADCAST_PROTECT	=101;
local 	BROADCAST_SURRENDER	=102;
local 	BROADCAST_JETTISON	=103;
local 	BROADCAST_TENSECONDS	=104;

local onFrameChanged = function (ship)
end

local onAICompleted = function (ship, ai_error)
	if ship~=nil and ship:exists() and ship==aship then
	--	aship:AIHoldPos()
	end
end

local CommLogic = function (ship)

	if Game.player:GetHale()==OPEN_COMM and ship:exists() then 
		Comms.ImportantMessage(Game.player.label..', Surrender your cargo, now, Respond!', ship.label)
		Game.player:SetCombatTarget(ship)
		Game.player:SetHale(ACCEPTED_COMM)
		ship:AIFlyTo(Game.player)
		playerhaled=true
	end

	if Game.time>timelimit and timelimit>0 then
		Game.player:SetHale(NO_DESTROY)
		timelimit=0
	end

	if Game.player:GetHale()==NO_DESTROY or Game.player:GetHale()==NOTIFY_POLICE and ship:exists() then 
		Game.player:SetCombatTarget(ship)
		playerhaled=false
		Comms.ImportantMessage(Game.player.label..', You will die!', ship.label)
		ship:AIKill(Game.player)
	end

	if Game.player:GetHale()==AGREED and ship:exists() then 
		if timelimit==0 then timelimit=Game.time+60 end
		Game.player:SetHale(AGREED)
		Game.player:SetCombatTarget(ship)
		playerhaled=true
		Comms.ImportantMessage(Game.player.label..', Come to a complete stop now!', ship.label)
		ship:AIFlyToClose(Game.player,100)
	end

	if Game.player:GetHale()==HALF_CARGO and ship:exists() then 
		if timelimit==0 then timelimit=Game.time+60 end
		Game.player:SetHale(HALF_CARGO)
		Game.player:SetCombatTarget(ship)
		playerhaled=true
		Comms.ImportantMessage(Game.player.label..', Come to a complete stop and jettison half your cargo!, You have '..math.floor(timelimit-Game.time)..' seconds!', ship.label)
		ship:AIFlyToClose(Game.player,100)
	end

	if Game.player:GetHale()==ALL_CARGO and ship:exists() then 
		if timelimit==0 then timelimit=Game.time+60 end
		Game.player:SetHale(ALL_CARGO)
		Game.player:SetCombatTarget(ship)
		playerhaled=true
		Comms.ImportantMessage(Game.player.label..', Come to a complete now and jettison all your cargo!, You have '..math.floor(timelimit-Game.time)..' seconds!', ship.label)
		ship:AIFlyToClose(Game.player,100)
	end
end


local onGameStart = function ()

Timer:CallEvery(10, function ()
	if aship==nil or not aship:exists() or not aship:isa('Ship') then

	local shipdefs = utils.build_array(utils.filter(function (k,def) return def.tag == 'SHIP' and def.hullMass <= 150 end, pairs(ShipDef)))
	if #shipdefs == 0 then return end
	local shipdef = shipdefs[Engine.rand:Integer(1,#shipdefs)]
	local default_drive = shipdef.defaultHyperdrive

		local max_laser_size = shipdef.capacity - EquipDef[default_drive].mass
		local laserdefs = utils.build_array(utils.filter(function (k, def) return def.slot == 'LASER' and def.mass <= max_laser_size and string.sub(def.id,0,11) == 'PULSECANNON' end, pairs(EquipDef)))
		local laserdef = laserdefs[Engine.rand:Integer(1,#laserdefs)]

		aship = Space.SpawnShipNear(shipdef.id, Game.player,10,20)
		aship:AddEquip(default_drive)
		aship:AddEquip(laserdef.id)
		aship:AddEquip("AUTOPILOT")
		aship:AIFlyTo(Game.player)
	end

	CommLogic(aship)

	if aship:exists() and playerhaled==false then

		if playerhaled then return end 

		Comms.ImportantMessage(Game.player.label..', Requesting channel!', aship.label)

		if not Game.player:GetCombatTarget() then Game.player:SetCombatTarget(aship) end
		Game.player:SetHale(BEING_HALED)
	end
end)
end

local onShipAlertChanged = function (ship, alert)


end

Event.Register("onShipAlertChanged", onShipAlertChanged)
Event.Register("onGameStart", onGameStart)
Event.Register("onAICompleted", onAICompleted)
