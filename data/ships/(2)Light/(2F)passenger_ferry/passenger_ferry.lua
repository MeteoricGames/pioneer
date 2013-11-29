-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_ship {
	name='Passenger Ferry',
	model='passenger_ferry',
	forward_thrust = 280e5,
	reverse_thrust = 280e5,
	up_thrust = 76e5,
	down_thrust = 76e5,
	left_thrust = 76e5,
	right_thrust = 76e5,
	angular_thrust = 780e5,
	max_cargo = 120,
	max_missile = 1,
	max_laser = 0,
	max_cargoscoop = 0,
	max_fuelscoop = 1,
	min_crew = 1,
	max_crew = 2,
	capacity = 120,
	hull_mass = 60,
	fuel_tank_mass = 20,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 57273e3,
	price = 300000,
	hyperdrive_class = 0,
}
