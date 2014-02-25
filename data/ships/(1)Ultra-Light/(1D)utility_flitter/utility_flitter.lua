-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Dragonfly',
	ship_class = 'utility_flitter',
	manufacturer = 'xian',
	model='utility_flitter',
	forward_thrust = 9e5,
	reverse_thrust = 9e5,
	up_thrust = 120e5,
	down_thrust = 120e5,
	left_thrust = 120e5,
	right_thrust = 120e5,
	angular_thrust = 1e5,
	max_cargo = 5,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 1,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 2,
	capacity = 5,
	hull_mass = 3,
	fuel_tank_mass = 1,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 140000,
	hyperdrive_class = 0,
	-- Paragon Flight System
    max_maneuver_speed = 500,
}
