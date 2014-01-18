-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local Engine = import("Engine")
local Lang = import("Lang")
local Game = import("Game")
local Comms = import("Comms")
local EquipDef = import("EquipDef")

local SmallLabeledButton = import("ui/SmallLabeledButton")
local InfoGauge = import("ui/InfoGauge")

local ui = Engine.ui
local l = Lang.GetResource("ui-core");

local x=0

local commDialog = function ()

	--init stuff
	local orbitalBody -- What we, or our space station, are orbiting
	local frameBody = Game.player.frameBody
	if not frameBody then
		-- Bug out if we're in a null frame. Save an embarrassing crash.
		return ui:Label(l.FAILED)
	end
	if frameBody.superType == 'STARPORT' then
		orbitalBody = Space.GetBody(frameBody.path:GetSystemBody().parent.index)
	else
		orbitalBody = frameBody
	end

	local distance = Game.player:DistanceTo(orbitalBody)
	local mass = orbitalBody.path:GetSystemBody().mass
	local radius = orbitalBody.path:GetSystemBody().radius
	local name = orbitalBody.label

	local G = 6.67428e-11

	local vCircular = math.sqrt((G * mass)/distance)
	local vEscape = math.sqrt((2 * G * mass)/distance)
	local vDescent = math.sqrt(G * mass * ((2 / distance) - (2 / (distance + radius))))

	--make buttons
	local deploySurvey = SmallLabeledButton.New("Deploy")
	local deployMiner  = SmallLabeledButton.New("Deploy")

	--widget updateable
	local miningListWidget = ui:Margin(0)

	--update function
	local cc = function()
	  return ui:Grid(1,2)
	     :SetRow(0, {
		ui:VBox(20):PackEnd({
            		(l.LOCATED_N_KM_FROM_THE_CENTRE_OF_NAME):interp({
				-- convert to kilometres
				distance = string.format('%6.2f',distance/1000),
				name = name
			}),
			ui:Label("Orbital data"):SetFont("HEADING_LARGE"),
			ui:Table():SetColumnSpacing(10):AddRows({
				-- convert to kilometres per second
				{ l.CIRCULAR_ORBIT_SPEED,    string.format('%6.2fkm/s',vCircular/1000) },
				{ l.ESCAPE_SPEED,            string.format('%6.2fkm/s',vEscape/1000)   },
				{ l.DESCENT_TO_GROUND_SPEED, string.format('%6.2fkm/s',vDescent/1000)  },
			}),
			ui:MultiLineText((l.ORBITAL_ANALYSIS_NOTES):interp({name = name})),
		})--packend
	     })--setrow
	     :SetRow(1, {
		ui:VBox(50):PackEnd({
			ui:Label("Comm stuff"):SetFont("HEADING_LARGE"),
				ui:Grid({10,10,50},1)
					:SetColumn(0, {
						ui:Table():SetRowSpacing(20):SetColumnSpacing(10):AddRows({
							{ "Clickable button :",x },
							{ "And another :",x },
						})	
					})
					:SetColumn(1, {
						ui:Table():SetRowSpacing(10):AddRows({
							{ deploySurvey.widget  }, 
							{ deployMiner.widget  }, 
						})
					}),
		})--packend
	     })--setrow
	end

	--make button logic
	local click = function ()
		x=x+1
		miningListWidget:SetInnerWidget(cc())
	end

	--make clickable area
	deploySurvey.button.onClick:Connect(click)
	deployMiner.button.onClick:Connect(click)

	--render ui
	return miningListWidget:SetInnerWidget(cc())
end

return commDialog
