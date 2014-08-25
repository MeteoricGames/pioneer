-- Copyright © 2013-2014 Meteoric Games Ltd

--Ships not available for purchase (ambient ships)
define_ship {
	name='Omni Explorer',
	ship_class = 'omni',
	manufacturer = 'alders_vectrum',
	model='omni_medium1',
	forward_thrust = 12000e5,
	reverse_thrust = 12000e5,
	up_thrust = 120000e5,
	down_thrust = 120000e5,
	left_thrust = 120000e5,
	right_thrust = 120000e5,
	angular_thrust = 30000e5,
	max_cargo = 6696,
	max_laser = 2,
	max_missile = 0,
	max_cargoscoop = 0,
	min_crew = 6,
	max_crew = 20,
	capacity = 6696,
	hull_mass = 3348,
	fuel_tank_mass = 1116,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 55123e3,
	price = 950000,
	hyperdrive_class = 13,
	-- Paragon Flight System
    max_maneuver_speed = 400,
}
