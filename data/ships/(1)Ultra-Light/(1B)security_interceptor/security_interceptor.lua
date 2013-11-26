-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Security Interceptor',
	model='security_interceptor',
	forward_thrust = 51e5,
	reverse_thrust = 51e5,
	up_thrust = 15e5,
	down_thrust = 15e5,
	left_thrust = 15e5,
	right_thrust = 15e5,
	angular_thrust = 15e5,
	max_cargo = 60,
	max_missile = 2,
	max_laser = 2,
	max_cargoscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 60,
	hull_mass = 6,
	fuel_tank_mass = 1,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 59167e3,
	price = 0,
	hyperdrive_class = 0,
}
