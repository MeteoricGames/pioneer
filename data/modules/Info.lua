local Lang = import("Lang")
local Comms = import("Comms")
local Event = import("Event")
local Game = import("Game")
local Space = import("Space")

function round(num, idp)
  local mult = 10^(idp or 0)
  return math.floor(num * mult + 0.5) / mult
end

local onEnterSystem = function (ship)
	if ship~=Game.player then return end
	Comms.ImportantInfo("System : "..Game.system.path:GetStarSystem().name..
		"\nFaction : "..Game.system.faction.name.."\nPopulation : "..round(Game.system.population,2)..
		" billion\nSecurity Rating : "..round(1-Game.system.lawlessness,2).."")
end
Event.Register("onEnterSystem", onEnterSystem)
