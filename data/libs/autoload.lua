-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Copyright © 2013-14 Meteoric Games Ltd
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

-- this is the only library automatically loaded at startup
-- its the right place to extend core Lua tables

local dev = import("Dev")
local Comms = import("Comms")
local Game = import("Game")

math.clamp = function(v, min, max)
	return math.min(max, math.max(v,min))
end

debug.deprecated = function()
	local deprecated_function = debug.getinfo(2)
	local caller = debug.getinfo(3)
	print("The use of the function \""..deprecated_function.name.."\" as done at <"..caller.source..":"..caller.currentline.."> is deprecated")
	print("Please check the changelogs and/or get in touch with the development team.")
end

-- a nice string interpolator
string.interp = function (s, t)
	return (s:gsub('(%b{})', function(w) return t[w:sub(2,-2)] or w end))
end

-- Money cheat
debug.giveme = function(m)
	game = import("Game")
	game.player:AddMoney(m)
end

_G.tweaker = {}

tweaker.list = function()
	out = dev:TweakerList()
	if out ~= nil then
		print(out)
	end
end

tweaker.tweak = function(t)
	out = dev:Tweak(t)
	if out ~= nil then
		print(out)
	end
end

tweaker.close = function()
	out = dev:TweakerClose()
	if out ~= nil then
		print(out)
	end
end

-- Spawn test cargo
debug.SpawnTestCargo = function(p)
	game = import("Game")
	out = game:SpawnTestCargo(p)
	if out ~= nil then print(out) end
end

function math.round(num, idp)
  local mult = 10^(idp or 0)
  return math.floor(num * mult + 0.5) / mult
end

debug.TestMultilineMessage = function()
	Comms.ImportantInfo("System : "..Game.system.path:GetStarSystem().name..
		"\nFaction : "..Game.system.faction.name.."\nPopulation : "..math.round(Game.system.population,2)..
		" billion\nSecurity Rating : "..math.round(1-Game.system.lawlessness,2).."")
end

debug.TestLineMessage = function()
	Comms.ImportantInfo("I HAVE GROWN TIRED WITH THIS STUPID PC NOT TURNING ON AFTER ELECTRICITY LOSS")
end

-- make import break. you should never import this file
return nil
