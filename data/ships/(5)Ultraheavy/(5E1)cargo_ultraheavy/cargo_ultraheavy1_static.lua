-- Copyright © 2013-2014 Meteoric Games Ltd

--Ships not available for purchase (ambient ships)
define_static_ship {
	name='Cargo Megafreighter',
	ship_class = 'cargo',
	manufacturer = 'alders_vectrum',
	model='cargo_ultraheavy1',
	forward_thrust = 427180e5,
	reverse_thrust = 427180e5,
	up_thrust = 180000e5,
	down_thrust = 180000e5,
	left_thrust = 180000e5,
	right_thrust = 180000e5,
	angular_thrust = 1800000e5,
	max_cargo = 256308,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 0,
	min_crew = 6,
	max_crew = 20,
	capacity = 256308,
	hull_mass = 128154,
	fuel_tank_mass = 42718,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 55123e3,
	price = 0,
	hyperdrive_class = 20,
}
