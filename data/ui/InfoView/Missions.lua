-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Game = import("Game")
local Format = import("Format")
local Character = import("Character")

local SmallLabeledButton = import("ui/SmallLabeledButton")
local SmartTable = import("ui/SmartTable")

local ui = Engine.ui
local l = Lang.GetResource("ui-core");

local c = {r = 0.0, g = 0.86, b = 1.0}
local expired_color = {r = 0.6, g = 0.6, b = 0.6}
local canceled_color = {r = 1.0, g = 0.2, b = 0.2}

-- we keep MissionList to remember players preferences
-- (now it is column he wants to sort by)
local MissionList 
local missions = function (tabGroup)
	-- This mission screen
	local MissionScreen = ui:Expand()

	if #Character.persistent.player.missions == 0 then
		return MissionScreen:SetInnerWidget( ui:Label(l.NO_MISSIONS):SetColor(c) )
	end

	local rowspec = {7,8,9,8,4,1,4,1,1,5} -- 7 columns
	if MissionList then 
		MissionList:Clear()
	else
		MissionList = SmartTable.New(rowspec) 
	end
	
	-- setup headers
	local headers = 
	{
		l.TYPE,
		l.CLIENT,
		l.LOCATION,
		l.DUE,
		l.REWARD,
		"",
		l.STATUS,
		"",
		"",
	}
	MissionList:SetHeaders(headers)
	
	-- we're not happy with default sort function so we specify one by ourselves
	local sortMissions = function (misList)
		local col = misList.sortCol
		local cmpByReward = function (a,b) 
			return a.data[col] >= b.data[col] 
		end
		-- Sort function to list Active missions first and sort missions by descending reward
		local cmpByStatus = function (a,b)
			if a.data[col] == b.data[col] then
				return a.data[col] < b.data[col] 
			else
				if a.data[col] == "ACTIVE" then
					return true
				else
					return false
				end
			end
		end
		local comparators = 
		{ 	-- by column num
			[5]	= cmpByReward,
			[6] = cmpByStatus,
		}
		misList:defaultSortFunction(comparators[col])
	end
	MissionList:SetSortFunction(sortMissions)
	MissionList:Sort(6)
	
	if Character.persistent.player.currentMission ~= nil then
		current_mission = Character.persistent.player.missions[Character.persistent.player.currentMission]
		if current_mission.status ~= "ACTIVE" then
			Character.persistent.player:SetCurrentMission(nil)
		end
	end

	for ref,mission in pairs(Character.persistent.player.missions) do
		if Character.persistent.player.currentMission == ref and mission.status ~= "ACTIVE" then
			Character.persistent.player:SetCurrentMission(nil)
		end
		if Character.persistent.player.currentMission == nil and mission.status == "ACTIVE" then
			Character.persistent.player:SetCurrentMission(ref)
		end
	
		-- Format the location
		local missionLocationName
		if mission.location.bodyIndex then
			missionLocationName = string.format('%s, %s [%d,%d,%d]', mission.location:GetSystemBody().name, mission.location:GetStarSystem().name, mission.location.sectorX, mission.location.sectorY, mission.location.sectorZ)
		else
			missionLocationName = string.format('%s [%d,%d,%d]', mission.location:GetStarSystem().name, mission.location.sectorX, mission.location.sectorY, mission.location.sectorZ)
		end
		-- Format the distance label
		local playerSystem = Game.system or Game.player:GetHyperspaceTarget()
		local dist = playerSystem:DistanceTo(mission.location)
		local distLabel = ui:Label(string.format('%.2f %s', dist, l.LY)):SetColor(c)
		local hyperjumpStatus = Game.player:GetHyperspaceDetails(mission.location)
		if hyperjumpStatus == 'CURRENT_SYSTEM' then
			distLabel:SetColor({ r = 0.0, g = 1.0, b = 0.2 }) -- green
		else
			if hyperjumpStatus == 'OK' then
				distLabel:SetColor({ r = 1.0, g = 1.0, b = 0.0 }) -- yellow
			else
				distLabel:SetColor({ r = 1.0, g = 0.0, b = 0.0 }) -- red
			end
		end
		-- Pack location and distance
		local locationBox = ui:VBox(2):PackEnd(ui:MultiLineText(missionLocationName))
									  :PackEnd(distLabel)
		
		-- Format Due info
		local dueLabel = ui:Label(Format.Date(mission.due)):SetColor(c)
		local duePeriod = mission.due - Game.time
		local days = math.max(0, duePeriod / (24*60*60))
		local daysLabel = ui:Label(string.format(l.D_DAYS_LEFT, days)):SetColor({ r = 1.0, g = 0.0, b = 1.0 }) -- purple
		local dueBox = ui:VBox(2):PackEnd(dueLabel):PackEnd(daysLabel)
		
		-- Buttons and Mission screen
		local missionStatusLabel = ui:Label("")
		local backButton = ui:Button(l.BACK):SetFont('HEADING_NORMAL')
		backButton.onClick:Connect(function()
			MissionScreen:SetInnerWidget(MissionList)
		end)		
		local cancelButton = ui:Button(ui:Label(l.CANCEL_MISSION):SetColor({r=1.0, g=0.0, b=0.0})):SetFont('HEADING_NORMAL')
		cancelButton.onClick:Connect(function()
			-- There should be a confirmation dialog here, similar to default controls one
			mission.status = "CANCELED"
			missionStatusLabel:SetText(l[mission.status])
			missionStatusLabel:SetColor(canceled_color)
			MissionScreen:SetInnerWidget(MissionList)
			tabGroup:Refresh()
		end)
		
		local moreButton = SmallLabeledButton.New(l.MORE_INFO)
		moreButton.button.onClick:Connect(function ()
			local buttons_box = ui:HBox(220)
			buttons_box:PackEnd(backButton)
			if mission.status == "ACTIVE" then
				buttons_box:PackEnd(cancelButton)
			end
			MissionScreen:SetInnerWidget(ui:VBox(10)
				:PackEnd({ui:Label(l.MISSION_DETAILS):SetFont('HEADING_LARGE'):SetColor(c)})
				:PackEnd((mission:GetClick())(mission))
				:PackEnd(buttons_box)
			)
		end)
		local moreButtonWidget = ui:Align("MIDDLE"):SetInnerWidget(moreButton)

		local description = mission:GetTypeDescription()		
		if duePeriod < 0 and mission.status == "ACTIVE" then
			mission.status = "EXPIRED"
		end
		missionStatusLabel:SetText((description and l[mission.status]) or l.INACTIVE)
		if mission.status == "EXPIRED" then
			missionStatusLabel:SetColor(expired_color)
		elseif mission.status == "CANCELED" then
			missionStatusLabel:SetColor(canceled_color)
		end
		missionStatusWidget = ui:Align("MIDDLE"):SetInnerWidget(missionStatusLabel)
		local imgPadding = ui:Image("icons/missions/padding.png", { "PRESERVE_ASPECT" }):SetTintColor(c)
		local imgLeft = ui:Image("icons/missions/current_mission_left.png", { "PRESERVE_ASPECT" }):SetTintColor(c)
		local imgRight = ui:Image("icons/missions/current_mission_right.png", { "PRESERVE_ASPECT" }):SetTintColor(c)
		local rowLeft = imgPadding
		local rowRight = imgPadding
		if mission.status == "ACTIVE" then
			if Character.persistent.player.currentMission == ref then
				missionStatusWidget:SetInnerWidget(
					missionStatusLabel
				)
				rowLeft = imgLeft
				rowRight = imgRight
			else
				local activateButton = ui:Button(ui:Label(l["ACTIVATE"]):SetColor({r=1.0, g=1.0, b=1.0}))
				missionStatusWidget:SetInnerWidget(activateButton)
				activateButton.onClick:Connect(function()
					Character.persistent.player:SetCurrentMission(ref)
					tabGroup:Refresh()
				end)
			end
		else
			missionStatusWidget = ui:Align("MIDDLE"):SetInnerWidget(
				missionStatusLabel
			)
		end
		
		local row =
		{ -- if we don't specify widget, default one will be used 
			{data = description or l.NONE},
			{data = mission.client.name},
			{data = dist, widget = locationBox},
			{data = mission.due, widget = dueBox},
			{data = mission.reward, widget = ui:Label(Format.Money(mission.reward)):SetColor({ r = 0.0, g = 1.0, b = 0.2 })}, -- green -- nil description means mission type isn't registered.
			{widget = ui:Align("MIDDLE"):SetInnerWidget(rowLeft)},
			{data = mission.status, widget = missionStatusWidget},
			{widget = ui:Align("MIDDLE"):SetInnerWidget(rowRight)},
			{widget = ui:Align("MIDDLE"):SetInnerWidget(imgPadding)},
			{widget = moreButtonWidget}
		}
		MissionList:AddRow(row)
	end

	MissionScreen:SetInnerWidget(MissionList)

	return MissionScreen
end

return missions
