// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GAMECONSTS_H
#define _GAMECONSTS_H

static const double PHYSICS_HZ = 60.0;

static const double MAX_LANDING_SPEED = 30.0;

static const Uint32 UNIVERSE_SEED = 0xabcd1234;

static const double EARTH_RADIUS = 6378135.0;
static const double EARTH_MASS   = 5.9742e24;
static const double SOL_RADIUS   = 6.955e8;
static const double SOL_MASS     = 1.98892e30;

static const double AU = 149598000000.0;
static const double G = 6.67428e-11;

static const double EARTH_ATMOSPHERE_SURFACE_DENSITY = 1.225;
static const double GAS_CONSTANT_R                   = 8.3144621;

static const int	EPOCH_START_YEAR = 2184;

// Save versions
// 74: First Steam release version (alpha 7)
// 75: +PlayerController read and write ShipController
// 76: +small starport upgraded to have more landing pads (LUA) -> BASE SAVE
// 77: +Docking AI changes to flyto, goto, and new dock command (WIP)
static const int  s_baseSaveVersion = 76;		
static const int  s_latestSaveVersion = 77;		


#endif /* _GAMECONSTS_H */
