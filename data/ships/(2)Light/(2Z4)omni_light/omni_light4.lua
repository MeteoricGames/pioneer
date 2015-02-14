-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='Terapin Light Ranger',
	ship_class = 'omni',
	manufacturer = 'alders_vectrum',
	model='omni_light4',
	cockpit='unitech_light_cockpit',
	forward_thrust = 538e5,
	reverse_thrust = 538e5,
	up_thrust = 5380e5,
	down_thrust = 5380e5,
	left_thrust = 5380e5,
	right_thrust = 5380e5,
	angular_thrust = 500e5,
	max_cargo = 324,
	max_missile = 1,
	max_laser = 2,
	max_cargoscoop = 0,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 3,
	capacity = 324,
	hull_mass = 162,
	hydrogen_tank = 100,
	fuel_tank_mass = 95,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 57273e3,
	price = 250000,
	hyperdrive_class = 6,
	-- Paragon Flight System
    max_maneuver_speed = 675,
}
