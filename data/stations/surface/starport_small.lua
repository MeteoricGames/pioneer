define_surface_station {
	model = 'starport_small',
    num_docking_ports = 4,
	-- define groups of bays, in this case 1 group with 1 bay.
	-- params are = {minSize, maxSize, {list,of,bay,numbers}}
	bay_groups = {
		{30, 60, {1}},
		{0, 25, {2}},
		{30, 60, {3}},
		{0, 25, {4}},
		{0, 25, {5}},
		{0, 25, {6}},
		{0, 25, {7}},
		{0, 25, {8}},
		{0, 25, {9}},
		{0, 25, {10}},
	},
    parking_distance = 5000.0,
    parking_gap_size = 2000.0,
    ship_launch_stage = 0,
    dock_anim_stage_duration = { 300, 4.0},
    undock_anim_stage_duration = { 0 },
}
