-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Event = import("Event")

local TabGroup = import("ui/TabGroup")

local commDialog = import("CommView/CommDialog")

local ui = Engine.ui
local l = Lang.GetResource("ui-core");

local tabGroup
ui.templates.CommView = function (args)
	if tabGroup then
		tabGroup:SwitchFirst()
		return tabGroup.widget
	end

	tabGroup = TabGroup.New()

	tabGroup:AddTab({ id = "commDialog",        title = "CommDialog",     icon = "Satellite", template = commDialog         })

	return tabGroup.widget
end

Event.Register("onGameEnd", function ()
	tabGroup = nil
end)
