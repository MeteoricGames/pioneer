-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.tx

local aship
local playerhaled=false


local onFrameChanged = function (ship)

end

local onGameStart = function ()

end

local CommLogic = function (ship)
	if Game.player:GetHale()==2 and ship:exists() then 
		Comms.ImportantMessage(Game.player.label..', Leave your ship now!!', ship.label)
		Game.player:SetHale(3)
		Game.player:SetCombatTarget(ship)
	end
	if Game.player:GetHale()>3 and ship:exists() then 
		ship:AIKill(Game.player)
		Game.player:SetHale(0)
		Game.player:SetCombatTarget(ship)
		playerhaled=false
	end
end

local onShipAlertChanged = function (ship, alert)

	if aship==nil or not aship:exists() or not aship:isa('Ship') then
		aship = Space.SpawnShipNear("kanara", Game.player,1000,2000)
		aship:AddEquip('PULSECANNON_DUAL_1MW')
		aship:AddEquip('LASER_COOLING_BOOSTER')
		aship:AddEquip('ATMOSPHERIC_SHIELDING')
		aship:AIFlyTo(Game.player)
	end

	if not ship:isa('Ship') then return end
	if not ship:exists() then return end

	if aship:exists() then

		CommLogic(aship)

		if playerhaled then return end 

		Comms.ImportantMessage(Game.player.label..', Requesting channel!', aship.label)

		if not Game.player:GetCombatTarget() then Game.player:SetCombatTarget(aship) end
		Game.player:SetHale(1)
		playerhaled=true
	end
end

Event.Register("onShipAlertChanged", onShipAlertChanged)
Event.Register("onGameStart", onGameStart)
