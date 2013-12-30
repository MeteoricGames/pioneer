-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Armed Transport',
	ship_class = 'passenger_shuttle_armed',
	manufacturer = 'alders_vectrum',
	model='armed_transport',
	forward_thrust = 10e5,
	reverse_thrust = 10e5,
	up_thrust = 3e5,
	down_thrust = 3e5,
	left_thrust = 3e5,
	right_thrust = 3e5,
	angular_thrust = 30e5,
	max_cargo = 6,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 0,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 2,
	capacity = 6,
	hull_mass = 3,
	fuel_tank_mass = 1,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 80000,
	hyperdrive_class = 0,
	-- Paragon Flight System
    max_maneuver_speed = 1000,
}
