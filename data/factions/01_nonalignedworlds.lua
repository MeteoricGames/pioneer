-- Copyright © 2013-14 Meteoric Games Ltd

local f = Faction:new('Non Aligned Worlds')
	:description_short('Non-Aligned is the descriptor to all the setlled words not part of one of the major colonial states.')
	:description('Non-Aligned worlds are those that are not part of any of the colonial states, generally settled later than the colonial states, they are clustered closely around Sol')
	:homeworld(0,0,0,0,4)
	:foundingDate(2900)
	:expansionRate(0.2)
	:military_name('Local Militia')
	:police_name('Police')
	:colour(0.0,1.0,0.3)

f:govtype_weight('EARTHDEMOC',    60)
f:govtype_weight('EARTHCOLONIAL', 40)

f:illegal_goods_probability('ANIMAL_MEAT',75)	-- fed/cis
f:illegal_goods_probability('LIVE_ANIMALS',75)	-- fed/cis
f:illegal_goods_probability('HAND_WEAPONS',100)	-- fed
f:illegal_goods_probability('BATTLE_WEAPONS',50)	--fed/cis
f:illegal_goods_probability('NERVE_GAS',100)--fed/cis
f:illegal_goods_probability('NARCOTICS',100)--fed
f:illegal_goods_probability('SLAVES',100)--fed/cis

f:add_to_factions('Non Aligned Worlds')
