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

local 	BROADCAST_ATTACK	=100;
local 	BROADCAST_PROTECT	=101;
local 	BROADCAST_SURRENDER	=102;
local 	BROADCAST_JETTISON	=103;
local 	BROADCAST_TENSECONDS	=104;

local onFrameChanged = function (ship)
end

local onAICompleted = function (ship, ai_error)
	if aship~=nil and aship:exits() and ship==aship then
		aship:AIHoldPos()
	end
end

local CommLogic = function (ship)
	if Game.player:GetHale()==OPEN_COMM and ship:exists() then 
		Comms.ImportantMessage(Game.player.label..', Leave your ship now!!', ship.label)
		Game.player:SetHale(ACCEPTED_COMM)
		Game.player:SetCombatTarget(ship)
	end

	if Game.player:GetHale()==NO_DESTROY or Game.player:GetHale()==NOTIFY_POLICE and aship:exists() then 
		aship:CancelAI()
		Game.player:SetHale(NO_COMM_OR_REJECT)
		Game.player:SetCombatTarget(ship)
		playerhaled=false
		Comms.ImportantMessage(Game.player.label..', Ok die then!', aship.label)
		aship:AIKill(Game.player)
	end

	if Game.player:GetHale()==AGREED and aship:exists() then 
		aship:CancelAI()
		Game.player:SetHale(NO_COMM_OR_REJECT)
		Game.player:SetCombatTarget(ship)
		playerhaled=true
		Comms.ImportantMessage(Game.player.label..', Come to a complete stop now!', aship.label)
		aship:AIFlyToClose(Game.player,200)
	end

	if Game.player:GetHale()==HALF_CARGO and aship:exists() then 
		aship:CancelAI()
		Game.player:SetHale(NO_COMM_OR_REJECT)
		Game.player:SetCombatTarget(ship)
		playerhaled=true
		Comms.ImportantMessage(Game.player.label..', Come to a complete stop now and jettison half your cargo!, You have 60 seconds!', aship.label)
		aship:AIFlyToClose(Game.player,200)
	end

	if Game.player:GetHale()==ALL_CARGO and aship:exists() then 
		aship:CancelAI()
		Game.player:SetHale(NO_COMM_OR_REJECT)
		Game.player:SetCombatTarget(ship)
		playerhaled=true
		Comms.ImportantMessage(Game.player.label..', Come to a complete stop now and jettison all your cargo!, You have 60 seconds!', aship.label)
		aship:AIFlyToClose(Game.player,200)
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
		playerhaled=true
	end
end)
end

local onShipAlertChanged = function (ship, alert)


end

Event.Register("onShipAlertChanged", onShipAlertChanged)
Event.Register("onGameStart", onGameStart)
Event.Register("onAICompleted", onAICompleted)
