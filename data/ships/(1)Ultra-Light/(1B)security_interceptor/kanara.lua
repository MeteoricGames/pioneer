-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Security Interceptor',
	ship_class = 'test',
	manufacturer = '1',
	model='security_interceptor',
	cockpit='passenger_shuttle_cockpit',
	forward_thrust = 9e5,
	reverse_thrust = 9e5,
	up_thrust = 6e5,
	down_thrust = 6e5,
	left_thrust = 6e5,
	right_thrust = 6e5,
	angular_thrust = 10e5,
	max_cargo = 6,
	max_missile = 2,
	max_laser = 2,
	max_cargoscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 6,
	hull_mass = 4,
	fuel_tank_mass = 2,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 59167e3,
	price = 0,
	hyperdrive_class = 0,
	-- Paragon Flight System
    max_maneuver_speed = 350,
}
