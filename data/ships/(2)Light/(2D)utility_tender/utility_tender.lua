-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Utility Tender',
	model='utility_tender',
	forward_thrust = 220e5,
	reverse_thrust = 220e5,
	up_thrust = 250e5,
	down_thrust = 250e5,
	left_thrust = 250e5,
	right_thrust = 250e5,
	angular_thrust = 400e5,
	max_cargo = 120,
	max_missile = 1,
	max_laser = 0,
	max_cargoscoop = 0,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 2,
	capacity = 672,
	hull_mass = 336,
	fuel_tank_mass = 112,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 57273e3,
	price = 450000,
	hyperdrive_class = 0,
}
