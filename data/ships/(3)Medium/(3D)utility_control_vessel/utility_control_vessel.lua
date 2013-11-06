-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

--Ships not available for purchase (ambient ships)
define_static_ship {
	name='Utility Control Vessel',
	model='utility_control_vessel',
	forward_thrust = 210036e5,
	reverse_thrust = 210036e5,
	up_thrust = 122521e5,
	down_thrust = 122521e5,
	left_thrust = 122521e5,
	right_thrust = 122521e5,
	angular_thrust = 350060e5,
	max_cargo = 16000,
	max_laser = 0,
	max_missile = 0,
	max_cargoscoop = 0,
	min_crew = 6,
	max_crew = 20,
	capacity = 16000,
	hull_mass = 70012,
	fuel_tank_mass = 6000,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 55123e3,
	price = 3.1e8,
	hyperdrive_class = 10,
}