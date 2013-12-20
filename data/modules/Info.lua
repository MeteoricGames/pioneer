local Lang = import("Lang")
local Comms = import("Comms")
local Event = import("Event")
local Game = import("Game")
local Space = import("Space")

local onEnterSystem = function (ship)
	if ship~=Game.player then return end
	Comms.ImportantInfo("System : xyz \nPopulation : xyz \nOther : styff")
	Comms.Info(" ")
end
Event.Register("onEnterSystem", onEnterSystem)
