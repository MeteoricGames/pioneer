
local miners = { }

local check = function(object)
	if object~=nil and object:exists() then return true end
	return false
end

local onJettison = function (ship, cargo) 
	if cargo == 'MINING_DRONE' then
		local miner = nil
		Timer:CallAt(Game.time+4, function ()

			local cargos = Space.GetBodies(function (body) return body:isa("CargoBody") and body:DistanceTo(Game.player)<100000 and body.type=='MINING_DRONE' end)

			if #cargos>0 then 
				miner = Space.SpawnShipNear('mining_drone', cargos[1], 0.00001, 0.00001)
				cargos[1]:Remove()
				cargos[1]=nil
				miner:AddEquip('MININGCANNON_17MW')
				miner:AddEquip('LASER_COOLING_BOOSTER')
				miner:AddEquip('ATMOSPHERIC_SHIELDING')
			else 
				return
			end

			miners[miner] = {
				status		= 'mining',
				miner		= miner,
				target		= Game.player.frameBody
			}
			local miningrobot = miners[miner].miner
		
			if check(miningrobot) then 
				miningrobot:AIFlyToClose(Game.player.frameBody,100)
				miningrobot:SetLabel('[mining drone]')
				Timer:CallEvery(4, function()
					if check(miningrobot) and  miners[miner].status == 'hold' then miningrobot:AIFire(miningrobot) end
				end)
			end
		end)
	end	
end
Event.Register("onJettison", onJettison)

local onAICompleted = function (ship, ai_error)
	if miners[ship]==nil then return end
	
	local miner = miners[ship]

	print('Miner status :'..miner.status)

	--drop it 2 seconds so it faces the correct way.
	Timer:CallAt(Game.time+2, function () 
	if check(miner.miner) and miner.status == 'mining' then
		miner.miner:AIHoldPos(miner.target)
		miners[ship] = {
			status		= 'hold',
			miner		= miner.miner,
			target		= miner.target
		}
	end
	end)

	Timer:CallAt(Game.time+600, function () 

	end)
end
Event.Register("onAICompleted", onAICompleted)
