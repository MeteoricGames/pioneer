-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Security Interceptor',
	ship_class = 'security_interceptor',
	manufacturer = 'alders_vectrum',
	model='security_interceptor',
	forward_thrust = 8e5,
	reverse_thrust = 8e5,
	up_thrust = 80e5,
	down_thrust = 80e5,
	left_thrust = 80e5,
	right_thrust = 80e5,
	angular_thrust = 40e5,
	max_cargo = 3,
	max_missile = 6,
	max_laser = 2,
	max_cargoscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 3,
	hull_mass = 2,
	fuel_tank_mass = 1,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 59167e3,
	price = 0,
	hyperdrive_class = 0,
	-- Paragon Flight System
        max_maneuver_speed = 700,
}