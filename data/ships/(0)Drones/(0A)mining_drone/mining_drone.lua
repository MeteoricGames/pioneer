-- Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
-- Licensed under the terms of CC-BY-SA 3.0. See licenses/CC-BY-SA-3.0.txt

define_drone {
	name='Mining Drone',
	model='mining_drone',
	forward_thrust = 52e5,
	reverse_thrust = 52e5,
	up_thrust = 28e5,
	down_thrust = 6e5,
	left_thrust = 6e5,
	right_thrust = 6e5,
	angular_thrust = 86e5,
	max_cargo = 30,
	max_laser = 1,
	max_missile = 0,
 	gun_mounts =
        {
                { v(0,0,0), v(0,0,-1), 5, 'HORIZONTAL' },
        },
	max_cargoscoop = 1,
	max_fuelscoop = 0,
	min_crew = 1,
	max_crew = 1,
	capacity = 30,
	hull_mass = 30,
	fuel_tank_mass = 25,
	-- Exhaust velocity Vc [m/s] is equivalent of engine efficiency and depend on used technology. Higher Vc means lower fuel consumption.
	-- Smaller ships built for speed often mount engines with higher Vc. Another way to make faster ship is to increase fuel_tank_mass.
	effective_exhaust_velocity = 80000e3,
	price = 0,
	hyperdrive_class = 0,
	max_engines = 0,
}
