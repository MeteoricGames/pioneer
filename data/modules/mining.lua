
local miners = { }

local onJettison = function (ship, cargo) 
	if cargo == 'MINING_DRONE' then
		local miner = nil
		Timer:CallAt(Game.time+4, function ()

			local cargos = Space.GetBodies(function (body) return body:isa("CargoBody") and body:DistanceTo(Game.player)<1000 and body.type=='MINING_DRONE' end)

			if #cargos>0 then 
				miner = Space.SpawnShipNear('mining_drone', cargos[1], 0.00001, 0.00001)
				cargos[1]:Remove()
				cargos[1]=nil
			end

			miners[miner] = {
				status		= 'mining',
				miner		= miner,
				target		= Game.player:GetNavTarget()
			}
			local miningrobot = miners[miner].miner
		
			if miningrobot~=nil then 
				miningrobot:AIFlyToClose(Game.player:GetNavTarget(),100)
				miningrobot:SetLabel('[mining drone]')
			--	cargo:Remove('ok')
				Timer:CallEvery(1, function()
					if miningrobot:exists() and  miners[miner].status == 'mining' then miningrobot:AIFire(miningrobot) end
				end)	
			end
		end)
	end	
end
Event.Register("onJettison", onJettison)

local onAICompleted = function (ship, ai_error)
	if miners[ship] == nil then return end
	
	local miner = miners[ship]

	print('Miner status :'..miner.status)

	Timer:CallAt(Game.time+4, function () 
	if miner.status == '___mining' then
		miner.miner:AIEnterLowOrbit(miner.target)
		miners[ship] = {
			status		= 'orbit',
			miner		= miner.miner,
			target		= miner.target
		}
	end
	end)

	Timer:CallAt(Game.time+4, function ()
	if miner.status == 'docked' then
		miner.miner:AIFlyToClose(miner.target,100)
		miners[ship] = {
			status		= 'deorbit',
			miner		= miner.miner,
			target		= miner.target
		}
	end
	end)

	Timer:CallAt(Game.time+4, function ()
	if miner.status == 'mining' then
		Timer:CallAt(Game.time+60, function ()
		miner.miner:AIFlyToClose(miner.target,100)
		miners[ship] = {
			status		= 'docked',
			miner		= miner.miner,
			target		= miner.target
		}
		end)
	end
	end)

	Timer:CallAt(Game.time+4, function ()
	if miner.status == 'deorbit' then
		miner.miner:AIFlyToClose(miner.target,100)
		miners[ship] = {
			status		= 'mining',
			miner		= miner.miner,
			target		= miner.target
		}
	end
	end)

end
--Event.Register("onAICompleted", onAICompleted)
