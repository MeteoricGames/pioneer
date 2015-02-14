-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Epsilon Corvette',
	ship_class = 'military',
	manufacturer = 'alders_vectrum',
	model='military_light2',
	cockpit='unitech_ultralight_cockpit',
	forward_thrust = 357e5,
	reverse_thrust = 357e5,
	up_thrust = 3570e5,
	down_thrust = 3570e5,
	left_thrust = 3570e5,
	right_thrust = 3570e5,
	angular_thrust = 500e5,
	max_cargo = 214,
	max_laser = 2,
	max_missile = 2,
	max_cargoscoop = 1,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 1,
	capacity = 214,
	hull_mass = 107,
	hydrogen_tank = 100,
	fuel_tank_mass = 36,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 60556e3,
	price = 325000,
	hyperdrive_class = 5,
	-- Paragon Flight System
    max_maneuver_speed = 700,
}
