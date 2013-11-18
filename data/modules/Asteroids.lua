-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt
--

local Engine = import("Engine")
local Game = import("Game")
local Space = import("Space")
local Comms = import("Comms")
local Timer = import("Timer")
local Event = import("Event")
local Serializer = import("Serializer")
local ShipDef = import("ShipDef")
local utils = import("utils")

local asteroids={}

local onEnterSystem = function (player)

end

local onGameStart = function ()

		local max = 200
		while max > 0  do
			max=max-1
			local ship = Space.SpawnShipNear("asteroid1", Game.player,1,20)
			if ship~=nil then
				ship:SetFuelPercent(100)
				asteroids[ship] = {
					ship	= ship,
				}
				ship:SetLabel('')
			end
		end

		Timer:CallEvery(1, function ()
			for k,v in pairs(asteroids) do
				if asteroids[k].ship~=nil and asteroids[k].ship:exists() and Game.player:DistanceTo(asteroids[k].ship) > 20000 then
					local x,y,z = Game.player:GetPos()
					y=y+100000000
					asteroids[k].ship:SetPos(Game.player,x,y,z)
					asteroids[k].ship:Explode()
					asteroids[k].ship=Space.SpawnShipNear("asteroid1", Game.player,10,20)
				end
			end
		end)
end

Event.Register("onGameStart", onGameStart)
Event.Register("onEnterSystem", onEnterSystem)
