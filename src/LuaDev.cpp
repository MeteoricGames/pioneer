// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "LuaDev.h"
#include "LuaObject.h"
#include "Pi.h"
#include "WorldView.h"
#include "Tweaker.h"

/*
 * Lua commands used in development & debugging
 * Everything here is subject to rapid changes
 */

/*
 * Set current camera offset to vector,
 * (the offset will reset when switching cameras)
 *
 * Dev.SetCameraOffset(x, y, z)
 */
static int l_dev_set_camera_offset(lua_State *l)
{
	if (!Pi::worldView)
		return luaL_error(l, "Dev.SetCameraOffset only works when there is a game running");
	CameraController *cam = Pi::worldView->GetCameraController();
	const float x = luaL_checknumber(l, 1);
	const float y = luaL_checknumber(l, 2);
	const float z = luaL_checknumber(l, 3);
	cam->SetPosition(vector3d(x, y, z));
	return 0;
}

static int l_dev_tweaker_list(lua_State *l)
{
	std::string out = Tweaker::LUAListTweaks();
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

static int l_dev_tweak(lua_State *l)
{
	const char *tweak_name = nullptr;
	if (lua_isnone(l, 1) || (tweak_name = lua_tostring(l, 2)) == nullptr) {
		return luaL_error(l, "Tweak takes one string argument (name of tweak)");
	}
	std::string out("Creating tweak...");
	if(Tweaker::Tweak(tweak_name)) {
		out += " ready";
	} else {
		out += " failed to create tweak. Use tweaker.list() to show all available tweaks";
	}
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

static int l_dev_tweaker_close(lua_State *l)
{
	Tweaker::Close();
	return 0;
}

static int l_dev_tweaker_monitor(lua_State *l)
{
	std::string out;
	if(Tweaker::Monitor()) {
		out = "Monitor mode enabled.";
	} else {
		out = "Monitor mode error.";
	}
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

void LuaDev::Register()
{
	lua_State *l = Lua::manager->GetLuaState();

	LUA_DEBUG_START(l);

	static const luaL_Reg methods[]= {
		{ "SetCameraOffset", l_dev_set_camera_offset },
		{ "TweakerList", l_dev_tweaker_list },
		{ "Tweak", l_dev_tweak },
		{ "TweakerClose", l_dev_tweaker_close },
		{ "TweakerMonitor", l_dev_tweaker_monitor },
		{ 0, 0 }
	};

	lua_getfield(l, LUA_REGISTRYINDEX, "CoreImports");
	luaL_newlib(l, methods);
	lua_setfield(l, -2, "Dev");
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);
}
