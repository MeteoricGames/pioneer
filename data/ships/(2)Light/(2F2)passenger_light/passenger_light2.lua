-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Ncom Interstar S3',
	ship_class = 'passenger',
	manufacturer = 'ncom',
	model='passenger_light2',
	forward_thrust = 202e5,
	reverse_thrust = 202e5,
	up_thrust = 2020e5,
	down_thrust = 2020e5,
	left_thrust = 2020e5,
	right_thrust = 2020e5,
	angular_thrust = 75e5,
	max_cargo = 120,
	max_missile = 1,
	max_laser = 0,
	max_cargoscoop = 0,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 2,
	capacity = 120,
	hull_mass = 61,
	hydrogen_tank = 100,
	fuel_tank_mass = 20,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 57273e3,
	price = 170000,
	hyperdrive_class = 3,
	-- Paragon Flight System
    max_maneuver_speed = 500,
}
