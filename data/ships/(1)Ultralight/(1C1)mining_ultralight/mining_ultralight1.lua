-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Sukhov UL Miner',
	ship_class = 'mining',
	manufacturer = 'sukhov',
	model='mining_ultralight1',
	cockpit='unitech_ultralight_cockpit',
	forward_thrust = 21e5,
	reverse_thrust = 21e5,
	up_thrust = 210e5,
	down_thrust = 210e5,
	left_thrust = 210e5,
	right_thrust = 210e5,
	angular_thrust = 2e5,
	max_cargo = 21,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 1,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 2,
	capacity = 21,
	hull_mass = 11,
	hydrogen_tank = 10,
	fuel_tank_mass = 4,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 50000,
	hyperdrive_class = 1,
	-- Paragon Flight System
    max_maneuver_speed = 500,
}
