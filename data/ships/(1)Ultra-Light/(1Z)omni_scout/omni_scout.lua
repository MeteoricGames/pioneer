-- Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Omni Scout',
	ship_class = 'omni_scout',
	manufacturer = 'alders_vectrum',
	model='omni_scout',
	forward_thrust = 165e5,
	reverse_thrust = 165e5,
	up_thrust = 1650e5,
	down_thrust = 1650e5,
	left_thrust = 1650e5,
	right_thrust = 1650e5,
	angular_thrust = 550e5,
	max_cargo = 99,
	max_laser = 2,
	max_missile = 4,
	max_cargoscoop = 0,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 99,
	hull_mass = 50,
	fuel_tank_mass = 17,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 65000e3,
	price = 200000,
	hyperdrive_class = 3,
	-- Paragon Flight System
    max_maneuver_speed = 700,
}
