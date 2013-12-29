-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Military Corvette',
	ship_class = 'test',
	manufacturer = '1',
	model='military_corvette',
	forward_thrust = 357e5,
	reverse_thrust = 357e5,
	up_thrust = 120e5,
	down_thrust = 120e5,
	left_thrust = 120e5,
	right_thrust = 120e5,
	angular_thrust = 1200e5,
	max_cargo = 214,
	max_laser = 2,
	max_missile = 2,
	max_cargoscoop = 1,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 1,
	capacity = 214,
	hull_mass = 107,
	fuel_tank_mass = 36,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 60556e3,
	price = 2870,
	hyperdrive_class = 5,
}
