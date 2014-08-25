-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Starling Shuttle',
	ship_class = 'passenger',
	manufacturer = 'alders_vectrum',
	model='passenger_ultralight3',
	cockpit='civilian_fighter2_cockpit',
	forward_thrust = 165e5,
	reverse_thrust = 165e5,
	up_thrust = 1650e5,
	down_thrust = 1650e5,
	left_thrust = 1650e5,
	right_thrust = 1650e5,
	angular_thrust = 1650e5,
	max_cargo = 80,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 0,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 80,
	hull_mass = 50,
	fuel_tank_mass = 17,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 50000,
	hyperdrive_class = 0,
	-- Paragon Flight System
    max_maneuver_speed = 650,
}
