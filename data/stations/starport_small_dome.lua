define_surface_station {
	model = 'starport_small_dome',
    num_docking_ports = 4,
	-- define groups of bays, in this case 1 group with 1 bay.
	-- params are = {minSize, maxSize, {list,of,bay,numbers}}
	bay_groups = {
		{20, 50, {1}},
		{20, 50, {2}},
		{20, 50, {3}},
		{20, 50, {4}},
	},
    parking_distance = 5000.0,
    parking_gap_size = 2000.0,
    ship_launch_stage = 0,
    dock_anim_stage_duration = { 300, 4.0},
    undock_anim_stage_duration = { 0 },
}
