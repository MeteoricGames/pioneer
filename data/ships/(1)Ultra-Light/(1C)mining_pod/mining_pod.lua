-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Mining Pod',
	ship_class = 'test',
	manufacturer = '1',
	model='mining_pod',
	forward_thrust = 21e5,
	reverse_thrust = 21e5,
	up_thrust = 7e5,
	down_thrust = 7e5,
	left_thrust = 7e5,
	right_thrust = 7e5,
	angular_thrust = 70e5,
	max_cargo = 21,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 1,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 2,
	capacity = 21,
	hull_mass = 11,
	fuel_tank_mass = 4,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 150000,
	hyperdrive_class = 1,
}
