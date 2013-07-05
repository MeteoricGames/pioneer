
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

				if not check(miner) then return end

				cargos[1]:Remove()
				cargos[1]=nil
				miner:AddEquip('MININGCANNON_17MW')
				miner:AddEquip('LASER_COOLING_BOOSTER')
				miner:AddEquip('ATMOSPHERIC_SHIELDING')

				-- set status
				local x,y,z = Game.player:GetPos()
				miners[miner] = {
					status		= 'mining',
					miner		= miner,
					target		= Game.player.frameBody,
					targetname	= Game.player.frameBody.path,
					ore		= '',
					system		= Game.system.path,
					spawnat		= Game.player:DistanceTo(Game.player.frameBody),
					started		= Game.time,
					metallicity	= Game.player.frameBody.path:GetSystemBodyMetallicity(),
					px		= x,
					py		= y,
					pz		= z
				}
				local miningrobot = miners[miner].miner
				doCollectOrMine(miningrobot)
			else 
				return
			end
		end)
	end	
end
Event.Register("onJettison", onJettison)

local onAICompleted = function (ship, ai_error)
	if miners==nil then return end
	if miners[ship]==nil then return end
	
	local miner = miners[ship]

	--drop it 2 seconds so it faces the correct way.
	Timer:CallAt(Game.time+2, function () 

	-- Are we full?
	if miner.miner:GetStats().freeCapacity == 0 and miner.status~='pickup' and miner.status~='pickup_ok' then
		miners[miner.miner].status = 'full'
		Comms.ImportantMessage('MINER: status cargo full, ready for pickup.', miner.miner.label)
	end

	if check(miner.miner) and miner.status 		== 'full' 
	then
		miner.miner:CancelAI()
		miner.miner:AIFlyToClose(miner.target,10000)
		miners[miner.miner].status = 'pickup'

	elseif check(miner.miner) and miner.status 	== 'pickup'
	then
		local m = Space.GetBodies(function (body) return body:isa("Ship") and body:DistanceTo(Game.player)<20000 and body~=Game.player end)

		if m~=nil and m[1] == miner.miner then
			miner.miner:CancelAI()
			miner.miner:AIFlyToClose(Game.player,50)
			miners[miner.miner].status = 'pickup_ok'
			Comms.ImportantMessage('MINER: inbound of cargo transfer.', miner.miner.label)
		else
			miner.miner:CancelAI()
			miner.miner:AIFlyToClose(miner.target,5000)
			miners[miner.miner].status = 'full'
		end

	elseif check(miner.miner) and miner.status 	== 'pickup_ok'
	then
		--transfer stuff
		--and go mining...
		
	--collect the ore, add to cargobay
	elseif check(miner.miner) and miner.status 	== 'collect' 
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
			started		= miner.started,
			metallicity	= miner.metallicity,
			px		= miner.px,
			py		= miner.py,
			pz		= miner.pz
		}
	end

	print('Miner status :'..miner.status)
	end)
end

local onEnterSystem = function (ship)
	if ship~=Game.player then return end
	
	if miners_remote~=nil then
		for k,v in pairs(miners_remote) do
			if v.system==Game.system.path then 
				print (v.started.."  - Fant en her - "..v.spawnat)

				local newtarget = Space.GetBody(v.targetname:GetSystemBody().index)
				--local newminer 	= Space.SpawnShipNear('mining_drone',newtarget,v.spawnat/1000,v.spawnat/1000)
				local newminer 	= Space.SpawnShipAtPos('mining_drone',newtarget,v.px,v.py,v.pz)

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
					started		= v.started,
					metallicity	= v.metallicity,
					px		= v.px,
					py		= v.py,
					pz		= v.pz
				}

				Game.player:SetCombatTarget(miners[newminer].miner)
				Game.player:SetNavTarget(miners[newminer].target)
				miners[newminer].miner:AIFlyToClose(newtarget,200)

				-- you were not here so we have to generate mined stuff..
				for x=1,Game.time-miners[newminer].started,30 do
					local x = Engine.rand:Integer(1,100)
					local m = miners[newminer].metallicity*100
					if 	20*x < m then
							miners[newminer].miner:AddEquip('PRECIOUS_METALS')
							print "Added Prscious metals"
					elseif 	8*x < m then
							miners[newminer].miner:AddEquip('METAL_ALLOYS')
							print "Added metal alloys"
					elseif	x < m then
							miners[newminer].miner:AddEquip('METAL_ORE')
							print "Added metal ore"
					elseif	x < m*2 then
							miners[newminer].miner:AddEquip('WATER')
							print "Added water"
					else
							miners[newminer].miner:AddEquip('RUBBISH')
							print "Added rubbish"
					end
				end

				
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

--[[local serialize = function ()
	onLeaveSystem(Game.player)
	return miners_remote
end

local unserialize = function (data)
	miners = { }
	miners_remote=data
	onEnterSystem(Game.player)
end --]]

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
--GetSystemBodyMetallicity()  -- Game.player.frameBody.path:GetSystemBodyMetallicity()
--
--Serializer:Register("Miners", serialize, unserialize)
--
--
--
--
--
