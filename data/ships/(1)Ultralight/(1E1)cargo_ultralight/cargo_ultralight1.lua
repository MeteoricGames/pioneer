-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Cargo Loader',
	ship_class = 'cargo',
	manufacturer = 'xian',
	model='cargo_ultralight1',
	forward_thrust = 120e5,
	reverse_thrust = 120e5,
	up_thrust = 1200e5,
	down_thrust = 1200e5,
	left_thrust = 1200e5,
	right_thrust = 1200e5,
	angular_thrust = 30e5,
	max_cargo = 120,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 1,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 2,
	capacity = 120,
	hull_mass = 60,
	fuel_tank_mass = 20,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 55000,
	hyperdrive_class = 3,
	-- Paragon Flight System
    max_maneuver_speed = 450,
}
