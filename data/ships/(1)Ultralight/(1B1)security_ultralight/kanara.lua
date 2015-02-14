-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Security Interceptor',
	ship_class = 'security',
	manufacturer = 'alders_vectrum',
	model='security_ultralight1',
	forward_thrust = 16e5,
	reverse_thrust = 16e5,
	up_thrust = 160e5,
	down_thrust = 160e5,
	left_thrust = 160e5,
	right_thrust = 160e5,
	angular_thrust = 40e5,
	max_cargo = 6,
	max_missile = 2,
	max_laser = 2,
	max_cargoscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 6,
	hull_mass = 4,
	hydrogen_tank = 10,
	fuel_tank_mass = 2,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 59167e3,
	price = 0,
	hyperdrive_class = 0,
	-- Paragon Flight System
    max_maneuver_speed = 700,
}
