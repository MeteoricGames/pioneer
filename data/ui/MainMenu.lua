-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Game = import("Game")
local Ship = import("Ship")
local Player = import("Player")
local SystemPath = import("SystemPath")
local ErrorScreen = import("ErrorScreen")

local ui = Engine.ui
local l = Lang.GetResource("ui-core");
local c = {r = 0.0, g = 0.86, b = 1.0}

local setupPlayerShip = function ()
	Game.player:SetShipType("passenger_shuttle")
	Game.player:SetLabel(Ship.MakeRandomLabel())
	Game.player:AddEquip("UNOCCUPIED_CABIN")
	Game.player:AddEquip("ATMOSPHERIC_SHIELDING")
	Game.player:AddEquip("AUTOPILOT")
	Game.player:AddEquip("SCANNER")
	Game.player:AddEquip("HYDROGEN", 2)
	Game.player:SetMoney(1000)
end

local loadGame = function (path)
	local ok, err = pcall(Game.LoadGame, path)
	if not ok then
		ErrorScreen.ShowError(l.COULD_NOT_LOAD_GAME .. err)
	end
end

local doLoadDialog = function ()
	ui:NewLayer(
		ui.templates.FileDialog({
			title       = l.LOAD,
			helpText    = l.SELECT_GAME_TO_LOAD,
			path        = "savefiles",
			selectLabel = l.LOAD_GAME,
			onSelect    = loadGame,
			onCancel    = function () ui:DropLayer() end
		})
	)
end

local doSettingsScreen = function()
	ui.layer:SetInnerWidget(
		ui.templates.Settings({
			closeButtons = {
				{ text = l.RETURN_TO_MENU, onClick = function () ui.layer:SetInnerWidget(ui.templates.MainMenu()) end }
			}
		})
	)
end

local buttonDefs = {
	{ l.START_AT_NEW_HOPE,      function () Game.StartGame(SystemPath.New(1,-1,0,0,4)) setupPlayerShip() end },
	{ l.LOAD_GAME,              doLoadDialog },
	{ l.OPTIONS,                doSettingsScreen },
	{ l.QUIT,                   function () Engine.Quit() end },
}


local buttonSet = {}
for i = 1,#buttonDefs do
	local def = buttonDefs[i]
	local button = ui:Button(ui:HBox():PackEnd(ui:Label(def[1]):SetColor(c)))
	button.onClick:Connect(def[2])
	if i < 10 then button:AddShortcut(i) end
	if i == 10 then button:AddShortcut("0") end
	buttonSet[i] = button
end

local menu = 
	ui:Grid(1, { 0.35, 0.45, 0.2 })
		:SetRow(0, {
			ui:Grid({ 0.1, 0.8, 0.1 }, 1)
				:SetCell(1, 0,
					ui:Align("MIDDLE",
						ui:Image("icons/menu_logo.png", { "PRESERVE_ASPECT" })
					)
				)
		})
		:SetRow(1, {
			ui:Grid(3,1)
				:SetColumn(1, {
					ui:Align("MIDDLE",
						ui:VBox(10):PackEnd(buttonSet):SetFont("HEADING_NORMAL")
					)
				} )
		})
		:SetRow(2, {
			ui:Grid({0.05, 0.3, 0.55, 0.1}, 1)
				:SetCell(1, 0,
					ui:Align("LEFT",
						ui:Image("icons/developer_logo.png", { "PRESERVE_ASPECT" })
					)
				)
				:SetCell(2, 0,
					ui:Align("RIGHT",
						ui:Label("(build: "..Engine.version..")"):SetFont("HEADING_XSMALL"):SetColor(c)
					)
				)
		})

ui.templates.MainMenu = function (args) return menu end
