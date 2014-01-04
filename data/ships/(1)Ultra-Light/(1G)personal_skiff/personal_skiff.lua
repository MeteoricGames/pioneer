-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Personal Skiff',
	ship_class = 'personal_skiff',
	manufacturer = 'alders_vectrum',
	model='personal_skiff',
	forward_thrust = 9e5,
	reverse_thrust = 9e5,
	up_thrust = 3e5,
	down_thrust = 3e5,
	left_thrust = 3e5,
	right_thrust = 3e5,
	angular_thrust = 30e5,
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
	fuel_tank_mass = 1,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 120000e3,
	price = 40000,
        -- Paragon Flight System
        max_maneuver_speed = 20000,
}
