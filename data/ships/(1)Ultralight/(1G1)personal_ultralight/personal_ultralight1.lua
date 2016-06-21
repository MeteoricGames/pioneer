-- Copyright © 2013-2014 Meteoric Games Ltd

define_ship {
	name='AV Skiff',
	ship_class = 'personal',
	manufacturer = 'alders_vectrum',
	model='personal_ultralight1',
	cockpit='default_cockpit',
	forward_thrust = 9e5,
	reverse_thrust = 9e5,
	up_thrust = 90e5,
	down_thrust = 90e5,
	left_thrust = 90e5,
	right_thrust = 90e5,
	angular_thrust = 1e5,
	max_cargo = 3,
	max_laser = 0,
	max_missile = 0,
	max_fuelscoop = 0,
	max_cargoscoop = 0,
	max_ecm = 0,
	min_crew = 1,
	max_crew = 1,
	hyperdrive_class = 0,
	capacity = 3,
	hull_mass = 2,
	hydrogen_tank = 10,
	fuel_tank_mass = 1,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 120000e3,
	price = 25000,
    -- Paragon Flight System
    max_maneuver_speed = 500,
}
