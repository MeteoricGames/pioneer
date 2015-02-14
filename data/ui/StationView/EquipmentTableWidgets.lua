-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Game = import("Game")
local EquipDef = import("EquipDef")
local Comms = import("Comms")
local utils = import("utils")

local l = Lang.GetResource("ui-core")

-- XXX equipment strings are in core. this sucks
local lcore = Lang.GetResource("core")

local c = {r = 0.0, g = 0.86, b = 1.0}

local ui = Engine.ui

local equipIcon = {
	HYDROGEN =              "Hydrogen",
	LIQUID_OXYGEN =         "Liquid_Oxygen",
	METAL_ORE =             "Metal_ore",
	CARBON_ORE =            "Carbon_ore",
	METAL_ALLOYS =          "Metal_alloys",
	PLASTICS =              "Plastics",
	FRUIT_AND_VEG =         "Fruit_and_Veg",
	ANIMAL_MEAT =           "Animal_Meat",
	LIVE_ANIMALS =          "Live_Animals",
	LIQUOR =                "Liquor",
	GRAIN =                 "Grain",
	TEXTILES =              "Textiles",
	FERTILIZER =            "Fertilizer",
	WATER =                 "Water",
	MEDICINES =             "Medicines",
	CONSUMER_GOODS =        "Consumer_goods",
	COMPUTERS =             "Computers",
	ROBOTS =                "Robots",
	PRECIOUS_METALS =       "Precious_metals",
	INDUSTRIAL_MACHINERY =  "Industrial_machinery",
	FARM_MACHINERY =        "Farm_machinery",
	MINING_MACHINERY =      "Mining_machinery",
	AIR_PROCESSORS =        "Air_processors",
	SLAVES =                "Slaves",
	HAND_WEAPONS =          "Hand_weapons",
	BATTLE_WEAPONS =        "Battle_weapons",
	NERVE_GAS =             "Nerve_Gas",
	NARCOTICS =             "Narcotics",
	MILITARY_FUEL =         "Military_fuel",
	RUBBISH =               "Rubbish",
	RADIOACTIVES =          "Radioactive_waste",
}

local defaultFuncs = {
	-- can we trade in this item
	canTrade = function (e)
		return EquipDef[e].purchasable and (EquipDef[e].slot == "CARGO" or EquipDef[e].slot == "HYDROGENTANK") and Game.system:IsCommodityLegal(e)
	end,

	-- how much of this item do we have in stock?
	getStock = function (e)
		return Game.player:GetDockedWith():GetEquipmentStock(e)
	end,

	-- what do we charge for this item?
	getPrice = function (e)
		return Game.player:GetDockedWith():GetEquipmentPrice(e)
	end,

	-- do something when a "buy" button is clicked
	-- return true if the buy can proceed
	onClickBuy = function (e)
		return true -- allow buy
	end,

	-- do something when a "sell" button is clicked
	-- return true if the buy can proceed
	onClickSell = function (e)
		return true -- allow sell
	end,

	-- do something when we buy this commodity
	bought = function (e)
		-- add one to our stock
		Game.player:GetDockedWith():AddEquipmentStock(e, 1)
	end,

	-- do something when we sell this items
	sold = function (e)
		Game.player:GetDockedWith():AddEquipmentStock(e, -1)
	end,
	
	bulk_bought = function (e, u)
		-- add u to our stock
		Game.player:GetDockedWith():AddEquipmentStock(e, u)
	end,
	
	bulk_sold = function (e, u)
		Game.player:GetDockedWith():AddEquipmentStock(e, -u)
	end,
}

local bulkItemType = "NULL" -- value of e passed to the last
local refreshTablesFunc = nil

local bulkSlider = function (getter, setter, range_max)
	local initial_value = 0
	local caption = "Unit"
	local slider = ui:HSlider()
	local label = ui:Label((initial_value + 1) .. " " .. caption):SetColor(c)
	slider:SetValue(initial_value)
	slider.onValueChanged:Connect(function (new_value)
			local value = math.floor(new_value * (range_max - 1)) + 1
			if value > 1 then
				caption = "Units"
			else
				caption = "Unit"
			end
			label:SetText(value .. " " .. caption)
			setter(new_value)
		end)
	return ui:VBox():PackEnd({label, slider})
end

local bulkDialog = function (funcs, action, e, range_max)	
	local units_count = 1
	local okButton = ui:Button(ui:Label(action):SetFont("HEADING_NORMAL"):SetColor(c))
	okButton.onClick:Connect(
		function()
			-- buy/sell items			
			if units_count > 0 then
				local player = Game.player
				local price = funcs.getPrice(e)
				
				if action == "Buy" then
					local actual_added = player:AddEquip(e, units_count)
					player:AddMoney(actual_added * -price)
					funcs.bulk_sold(e, actual_added)				
				else 
					local actual_removed = player:RemoveEquip(e, units_count)
					player:AddMoney(units_count * price)
					funcs.bulk_bought(e, actual_removed)
				end
				refreshTablesFunc()
			end
			ui:DropLayer()
		end
	)
	local cancelButton = ui:Button(ui:Label("Cancel"):SetFont("HEADING_NORMAL"):SetColor(c))
	cancelButton.onClick:Connect(
		function()
			ui:DropLayer()
		end
	)
	
	local bulkGetter = function ()
		return 0
	end
	
	local bulkSetter = function (value)
		units_count = math.floor(value * (range_max - 1)) + 1
	end
	
	local dialog = 
		ui:ColorBackground(0, 0, 0, 0.5,
			ui:Align("MIDDLE",
				ui:Grid({2, 3, 2}, 1)
					:SetCell(1, 0, 
						ui:Background( 
							ui:VBox(10)
								:PackEnd(ui:Label(lcore[e]):SetFont("HEADING_NORMAL"):SetColor(c))
								:PackEnd(ui:VBox(5):PackEnd(bulkSlider(bulkGetter, bulkSetter, range_max)))
								:PackEnd(ui:HBox(10):PackEnd({okButton, cancelButton}))
						)
					)
			)
		)
	return dialog
end

local stationColumnHeading = {
	icon  = "",
	name  = l.NAME_OBJECT,
	price = l.PRICE,
	stock = l.IN_STOCK,
	mass  = l.MASS,
	buy_bulk = "",
}
local shipColumnHeading = {
	icon      = "",
	name      = l.NAME_OBJECT,
	amount    = l.AMOUNT,
	mass      = l.MASS,
	massTotal = l.TOTAL_MASS,
	sell_bulk = "",
}

local flagBulkOperation = false

local stationColumnValue = {
	icon  = function (e, funcs) return equipIcon[e] and ui:Image("icons/goods/"..equipIcon[e]..".png") or "" end,
	name  = function (e, funcs) return lcore[e] end,
	price = function (e, funcs) return string.format("%0.2f", funcs.getPrice(e)) end,
	stock = function (e, funcs) return funcs.getStock(e) end,
	mass  = function (e, funcs) return string.format("%dt", EquipDef[e].mass) end,
	buy_bulk = function (e, funcs) 
		local player = Game.player	
		local station_stock = funcs.getStock(e)
		if station_stock <= 1 then 
			return nil 
		end
		local btn = ui:Button(ui:Label("Buy.."))
		btn.onClick:Connect(
			function()
				flagBulkOperation = true
				
				if station_stock <= 0 then
					Comms.Message(l.ITEM_IS_OUT_OF_STOCK)
					return
				else 				
					local player_free_equip = player:GetEquipFree(EquipDef[e].slot)
					local player_free_capacity = player.freeCapacity
					local player_free_hydrogen_capacity = player:GetHydrogenFree()
					local player_money = player:GetMoney()
					local item_price = funcs.getPrice(e)
					local item_mass = EquipDef[e].mass
					
					if e ~= "HYDROGEN" then
						if  player_free_equip < 1 then
							Comms.Message(l.SHIP_IS_FULLY_LADEN)
							return
						end
						
						if player_free_capacity < item_mass then
							Comms.Message(l.SHIP_IS_FULLY_LADEN)
							return
						end
					else						
						if player_free_hydrogen_capacity < 1 then
							Comms.Message(l.SHIP_HYDROGEN_TANK_FULL)
							return
						end
					end
					if player_money < item_price then
						Comms.Message(l.YOU_NOT_ENOUGH_MONEY)
						return
					end
					
					-- Limit by: 
					-- * station stock
					-- * free capacity (mass)
					-- * money
					local limit_table = {}
					
					if e ~= "HYDROGEN" then
						limit_table = {
							station_stock,
							player_free_capacity / item_mass,
							player_money / item_price,
						}
					else
						limit_table = {
							station_stock,
							player_free_hydrogen_capacity,
							player_money / item_price,
						}
					end
					table.sort(limit_table)
					
					local buy_dialog = bulkDialog(funcs, "Buy", e, limit_table[1])
					ui:NewLayer(buy_dialog)
				end
			end
		)
		return btn
	end,
}
local shipColumnValue = {
	icon      = function (e, funcs) return equipIcon[e] and ui:Image("icons/goods/"..equipIcon[e]..".png") or "" end,
	name      = function (e, funcs) return lcore[e] end,
	amount    = function (e, funcs) return Game.player:GetEquipCount(EquipDef[e].slot, e) end,
	mass      = function (e, funcs) return string.format("%dt", EquipDef[e].mass) end,
	massTotal = function (e, funcs) return string.format("%dt", Game.player:GetEquipCount(EquipDef[e].slot,e)*EquipDef[e].mass) end,
	sell_bulk = function (e, funcs)
		local player = Game.player
		local item_count = player:GetEquipCount(EquipDef[e].slot, e)
		if item_count <= 1 then
			return nil
		end
		local btn = ui:Button(ui:Label("Sell.."))
		btn.onClick:Connect(
			function()
				-- This removes the 1 unit buy the click will cause
				flagBulkOperation = true
				local sell_dialog = bulkDialog(funcs, "Sell", e, item_count)
				ui:NewLayer(sell_dialog)
			end
		)
		return btn 
	end,
}

local EquipmentTableWidgets = {}

function EquipmentTableWidgets.Pair (config)
	local funcs = {
		canTrade = config.canTrade or defaultFuncs.canTrade,
		getStock = config.getStock or defaultFuncs.getStock,
		getPrice = config.getPrice or defaultFuncs.getPrice,
		onClickBuy = config.onClickBuy or defaultFuncs.onClickBuy,
		onClickSell = config.onClickSell or defaultFuncs.onClickSell,
		bought = config.bought or defaultFuncs.bought,
		sold = config.sold or defaultFuncs.sold,
		bulk_bought = defaultFuncs.bulk_bought,
		bulk_sold = defaultFuncs.bulk_sold,
	}

	local equipTypes = {}
	for k,v in pairs(EquipDef) do
		if funcs.canTrade(v.id) and k ~= "NONE" then
			table.insert(equipTypes, k)
		end
	end
	table.sort(equipTypes)

	local stationTable =
		ui:Table()
			:SetRowSpacing(5)
			:SetColumnSpacing(10)
			:SetHeadingRow(utils.build_table(utils.map(function (k,v) return k,stationColumnHeading[v] end, ipairs(config.stationColumns))))
			:SetHeadingFont("LARGE")
			:SetRowAlignment("CENTER")
			:SetMouseEnabled(true)

	local function fillStationTable ()
		stationTable:ClearRows()

		local rowEquip = {}
		local row = 1
		for i=1,#equipTypes do
			local e = equipTypes[i]
			stationTable:AddRow(utils.build_table(utils.map(function (k,v) return k,stationColumnValue[v](e,funcs) end, ipairs(config.stationColumns))))
			rowEquip[row] = e
			row = row + 1
		end

		return rowEquip
	end
	local stationRowEquip = fillStationTable()

	local shipTable =
		ui:Table()
			:SetRowSpacing(5)
			:SetColumnSpacing(10)
			:SetHeadingRow(utils.build_table(utils.map(function (k,v) return k,shipColumnHeading[v] end, ipairs(config.shipColumns))))
			:SetHeadingFont("LARGE")
			:SetRowAlignment("CENTER")
			:SetMouseEnabled(true)

	local function fillShipTable ()
		shipTable:ClearRows()

		local rowEquip = {}
		local row = 1
		for i=1,#equipTypes do
			local e = equipTypes[i]
			local n = Game.player:GetEquipCount(EquipDef[e].slot, e)
			if n > 0 then
				local icon = equipIcon[e] and ui:Image("icons/goods/"..equipIcon[e]..".png") or ""
				shipTable:AddRow(utils.build_table(utils.map(function (k,v) return k,shipColumnValue[v](e, funcs) end, ipairs(config.shipColumns))))
				rowEquip[row] = e
				row = row + 1
			end
		end

		return rowEquip
	end
	local shipRowEquip = fillShipTable()
	
	local function refreshTables()
		stationRowEquip = fillStationTable()
		shipRowEquip = fillShipTable()
	end
	refreshTablesFunc = refreshTables

	local function onBuy (e)
		if not funcs.onClickBuy(e) then return end
		if flagBulkOperation then
			flagBulkOperation = false
			return
		end

		if funcs.getStock(e) <= 0 then
			Comms.Message(l.ITEM_IS_OUT_OF_STOCK)
			return
		end

		local player = Game.player

		--if EquipDef[e].
		
		if e ~= "HYDROGEN" then 
			if player:GetEquipFree(EquipDef[e].slot) < 1 then
				Comms.Message(l.SHIP_IS_FULLY_LADEN)
				return
			end

			if player.freeCapacity < EquipDef[e].mass then
				Comms.Message(l.SHIP_IS_FULLY_LADEN)
				return
			end
		else
			if player:GetHydrogenFree() < 1 then
				Comms.Message(l.SHIP_HYDROGEN_TANK_FULL)
				return
			end
		end

		local price = funcs.getPrice(e)
		if player:GetMoney() < funcs.getPrice(e) then
			Comms.Message(l.YOU_NOT_ENOUGH_MONEY)
			return
		end

		assert(player:AddEquip(e) == 1)
		player:AddMoney(-price)
		--if player:AddEquip(e) == 1 then
		--	player:AddMoney(-price)
		--end
		funcs.sold(e)
	end

	local function onSell (e)
		if not funcs.onClickSell(e) then return end
		if flagBulkOperation then
			flagBulkOperation = false				
			stationRowEquip = fillStationTable()
			shipRowEquip = fillShipTable()
			return
		end

		local player = Game.player

		player:RemoveEquip(e)
		player:AddMoney(funcs.getPrice(e))

		funcs.bought(e)
	end

	stationTable.onRowClicked:Connect(function(row)
		local e = stationRowEquip[row+1]

		onBuy(e)

		refreshTables()
	end)

	shipTable.onRowClicked:Connect(function (row)
		local e = shipRowEquip[row+1]

		onSell(e)
		refreshTables()
	end)

	return stationTable, shipTable
end

return EquipmentTableWidgets
