
local miners = { }
local miners_remote = { }

local check = function(object)
	if object~=nil and object:exists() then return true end
	return false
end


local doCollectOrMine = function(miningrobot)
		if check(miningrobot) then 
			-- descend to 100m
			miningrobot:AIFlyToClose(miners[miningrobot].target,100)
			miningrobot:SetLabel('[mining drone]')
			Timer:CallEvery(2, function()
				if check(miningrobot) and  miners[miningrobot].status == 'hold' then  
					local ore = Space.GetBodies(function (body) return body:isa("CargoBody") and body:DistanceTo(miningrobot)<20000 and body.type~='MINING_DRONE' end)
					if #ore>0 then
						-- if floating ore do collect
						miners[miningrobot].status='collect'
						miningrobot:CancelAI()
						miningrobot:AIFlyToClose(ore[1],20)
						miners[miningrobot].ore=ore[1]
					else
						-- mine
						miningrobot:AIFire(miningrobot)
					end
				end
			end)
		end
end

local onJettison = function (ship, cargo) 
	if cargo == 'MINING_DRONE' then
		local miner = nil
		Timer:CallAt(Game.time+4, function ()
			
			-- deploy jettisoned miner.
			local cargos = Space.GetBodies(function (body) return body:isa("CargoBody") and body:DistanceTo(Game.player)<100000 and body.type=='MINING_DRONE' end)

			if #cargos>0 then 
			-- acivate miner
				miner = Space.SpawnShipNear('mining_drone', cargos[1], 0.00001, 0.00001)
				cargos[1]:Remove()
				cargos[1]=nil
				miner:AddEquip('MININGCANNON_17MW')
				miner:AddEquip('LASER_COOLING_BOOSTER')
				miner:AddEquip('ATMOSPHERIC_SHIELDING')
			else 
				return
			end

			-- set status
			miners[miner] = {
				status		= 'mining',
				miner		= miner,
				target		= Game.player.frameBody,
				targetname	= Game.player.frameBody.path,
				ore		= '',
				system		= Game.system.path,  
				spawnat		= Game.player:DistanceTo(Game.player.frameBody),
				started		= Game.time
			}
			local miningrobot = miners[miner].miner
			doCollectOrMine(miningrobot)
		end)
	end	
end
Event.Register("onJettison", onJettison)

local onAICompleted = function (ship, ai_error)
	if miners==nil then return end
	if miners[ship]==nil then return end
	
	local miner = miners[ship]

	print('Miner status :'..miner.status)

	--drop it 2 seconds so it faces the correct way.
	Timer:CallAt(Game.time+2, function () 

	--collect the ore, add to cargobay
	if check(miner.miner) and miner.status 		== 'collect' 
	then
		if miners[miner.miner].ore:exists() 
		then
			miner.miner:AddEquip(miners[miner.miner].ore.type)
			miners[miner.miner].ore:Remove()
			miner.miner:SetHullPercent(100)
		end
		miners[miner.miner].status = 'mining_prepare'
		miner.miner:CancelAI()
		miner.miner:AIFlyToClose(miner.target,200)
	--goto mining altitude
	elseif check(miner.miner) and miner.status 	== 'mining_prepare' 
	then
		miners[miner.miner].status = 'mining_prepare2'
		miner.miner:CancelAI()
		miners[miner.miner].miner:AIFlyToClose(miner.target,150)
	--goto mining altitude
	elseif check(miner.miner) and miner.status 	== 'mining_prepare2' 
	then
		miners[miner.miner].status = 'mining'
		miner.miner:CancelAI()
		miners[miner.miner].miner:AIFlyToClose(miner.target,100)
	--hold mining position
	elseif check(miner.miner) and miner.status 	== 'mining' 
	then
		miner.miner:AIHoldPos(miner.target)
		miners[ship] = {
			status		= 'hold',
			miner		= miner.miner,
			target		= miner.target,
			targetname	= miner.targetname,
			ore 		= '',
			system		= miner.system,
			spawnat		= miner.spawnat,
			started		= miner.started
		}
	end
	end)
end

local onEnterSystem = function (ship)
	if ship~=Game.player then return end
	
	if miners_remote~=nil then
		for k,v in pairs(miners_remote) do
			if v.system==Game.system.path then 
				print (v.started.."  - Fant en her - "..v.spawnat)

				local newtarget = Space.GetBody(v.targetname:GetSystemBody().index)
				local newminer 	= Space.SpawnShipNear('mining_drone',newtarget,v.spawnat/1000,v.spawnat/1000)
				
				newminer:AddEquip('MININGCANNON_17MW')
				newminer:AddEquip('LASER_COOLING_BOOSTER')
				newminer:AddEquip('ATMOSPHERIC_SHIELDING')
				newminer:SetLabel('[mining drone]')

				miners[newminer] = {
					status		= 'mining_prepare',
					miner		= newminer,
					target		= newtarget,
					targetname	= v.targetname,
					ore 		= '',
					system		= v.system,
					spawnat		= v.spawnat,
					started		= v.started
				}

				Game.player:SetCombatTarget(miners[newminer].miner)
				Game.player:SetNavTarget(miners[newminer].target)
				miners[newminer].miner:AIFlyToClose(newtarget,200)
				
			--	table.insert(miners,
			--	miners[miners[k].miner] = miners[k]

				doCollectOrMine(miners[newminer].miner)

				miners_remote[v.miner]=nil
			end
		end
	end
end

local onLeaveSystem = function (ship)
	if ship~=Game.player then return end
	if miners==nil then return end
	for k,v in pairs(miners) do miners_remote[k] = v end
	miners = { }
end

local serialize = function ()
	onLeaveSystem(Game.player)
	return miners_remote
end

local unserialize = function (data)
	miners = { }
	miners_remote=data
	onEnterSystem(Game.player)
end

local onGameEnd = function ()
	-- drop the references for our data so Lua can free them
	-- and so we can start fresh if the player starts another game
	miners,miners_remote=nil,nil
end

Event.Register("onGameEnd", onGameEnd)
Event.Register("onAICompleted", onAICompleted)
Event.Register("onLeaveSystem", onLeaveSystem)
Event.Register("onEnterSystem", onEnterSystem)

--Space.GetBody(Game.system.path.systemIndex)
--
--a=Game.player.frameBody.path:GetSystemBody()
--b=Space.GetBody(a.index)
--
Serializer:Register("Miners", serialize, unserialize)
--
--
--
--
--
