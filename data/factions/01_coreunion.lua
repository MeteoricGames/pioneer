-- Copyright Â© 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Copyright © 2013-14 Meteoric Games Ltd
-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

local f = Faction:new('United Systems of America')
	:description_short('One of the two interstellar superpowers.')
	:description('The United Systems of America is the successor state to the United States of America, founded primarily by American colonists, it is one of the most powerful colonial groupings, retaining its Earth age status as a superpower.')
	:homeworld(0,0,0,0,4)
	:foundingDate(2510)
	:expansionRate(0.2)
	:military_name('Union Navy')
	:police_name('Police')
	:colour(1.0,0.4,0.0)

f:govtype_weight('EARTHDEMOC',    60)
f:govtype_weight('EARTHCOLONIAL', 40)

f:illegal_goods_probability('ANIMAL_MEAT',75)	-- fed/cis
f:illegal_goods_probability('LIVE_ANIMALS',75)	-- fed/cis
f:illegal_goods_probability('HAND_WEAPONS',100)	-- fed
f:illegal_goods_probability('BATTLE_WEAPONS',50)	--fed/cis
f:illegal_goods_probability('NERVE_GAS',100)--fed/cis
f:illegal_goods_probability('NARCOTICS',100)--fed
f:illegal_goods_probability('SLAVES',100)--fed/cis

f:add_to_factions('Core Union')
