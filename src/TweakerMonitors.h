// Copyright � 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt


#ifndef _TWEAKER_MONITORS_H_
#define _TWEAKER_MONITORS_H_

#include "Ship.h"

// Structures used for monitors storage/updates
static const char* AICOMMAND_FLYTO_PERMACLOUD = "flytopermacloud";
static const char* AICOMMAND_CLEARAI = "clearai";

struct TW_AICOMMAND
{
	std::string command;
	Ship* ship;
};

struct TM_SHIP
{
	TM_SHIP() {
		Name = "";
		Velocity = 0.0;
		Direction.x = Direction.y = Direction.z = 0.0;
	}
	std::string Name;
	double Velocity;
	vector3d Direction;
	std::string AIStatus;
	std::string ModuleName;
	std::string ModuleStatus;
};

struct TM_HYPERCLOUD
{
	TM_HYPERCLOUD() 
	{
		HasShip = false;
	}
	std::string Type;
	std::string ShipName;
	double Due;
	double GameTime;
	bool HasShip;
};

#endif // TWEAKER_MONITORS_H