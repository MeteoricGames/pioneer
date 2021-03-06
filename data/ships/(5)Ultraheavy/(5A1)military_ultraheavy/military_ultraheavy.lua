-- Copyright © 2013-2014 Meteoric Games Ltd

--Ships not available for purchase (ambient ships)
define_ship {
	name='military carrier',
	ship_class = 'military',
	manufacturer = 'alders_vectrum',
	model='military_ultraheavy',
	forward_thrust = 692000e5,
	reverse_thrust = 692000e5,
	up_thrust = 200000e5,
	down_thrust = 200000e5,
	left_thrust = 200000e5,
	right_thrust = 200000e5,
	angular_thrust = 2000000e5,
	max_cargo = 415200,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 0,
	min_crew = 6,
	max_crew = 20,
	capacity = 415200,
	hull_mass = 207600,
	fuel_tank_mass = 69200,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 55123e3,
	price = 0,
	hyperdrive_class = 20,
	-- Paragon Flight System
    max_maneuver_speed = 150,
}
