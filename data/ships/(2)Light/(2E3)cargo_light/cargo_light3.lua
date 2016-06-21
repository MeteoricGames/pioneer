-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Sukhov LCH 800',
	ship_class = 'cargo',
	manufacturer = 'sukhov',
	model='cargo_light3',
	forward_thrust = 1120e5,
	reverse_thrust = 1120e5,
	up_thrust = 11200e5,
	down_thrust = 11200e5,
	left_thrust = 11200e5,
	right_thrust = 11200e5,
	angular_thrust = 500e5,
	max_cargo = 672,
	max_missile = 1,
	max_laser = 0,
	max_cargoscoop = 0,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 2,
	capacity = 672,
	hull_mass = 336,
	hydrogen_tank = 100,
	fuel_tank_mass = 112,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 57273e3,
	price = 135000,
	hyperdrive_class = 9,
	-- Paragon Flight System
    max_maneuver_speed = 300,
}
