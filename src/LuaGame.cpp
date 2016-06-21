
// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "LuaGame.h"
#include "LuaObject.h"
#include "LuaUtils.h"
#include "FileSystem.h"
#include "Player.h"
#include "Pi.h"
#include "Game.h"
#include "Lang.h"
#include "StringF.h"
#include "WorldView.h"
#include <sstream>
#include "LuaEvent.h"

/*
 * Interface: Game
 *
 * A global table that exposes a number of essential values relevant to the
 * current game.
 *
 */

/*
 * Function: StartGame
 *
 * Start a new game.
 *
 * > Game.StartGame(system_path, start_time)
 *
 * Parameters:
 *
 *   system_path - A SystemBody to start at. If this is a starport, the player
 *                 will begin docked here; otherwise the player will begin in
 *                 orbit around the specified body.
 *
 *   start_time - optional, default 0. Time to start at in seconds from the
 *                Paragon epoch (i.e. from 2184-01-01 00:00 UTC).
 *
 * Availability:
 *
 *   alpha 28
 *
 * Status:
 *
 *   experimental
 */
static int l_game_start_game(lua_State *l)
{
	if (Pi::game) {
		luaL_error(l, "can't start a new game while a game is already running");
		return 0;
	}

	SystemPath *path = LuaObject<SystemPath>::CheckFromLua(1);
	const double start_time = luaL_optnumber(l, 2, 0.0);

	RefCountedPtr<StarSystem> system(StarSystemCache::GetCached(*path));
	SystemBody *sbody = system->GetBodyByPath(path);
	if (sbody->GetSuperType() == SystemBody::SUPERTYPE_STARPORT)
		Pi::game = new Game(*path, start_time);
	else
		Pi::game = new Game(*path, vector3d(0, 1.5*sbody->GetRadius(), 0), start_time);

	return 0;
}

/*
 * Function: LoadGame
 *
 * Load a saved game.
 *
 * > Game.LoadGame(filename)
 *
 * Parameters:
 *
 *   filename - Filename to load. It will be loaded from the 'savefiles'
 *              directory in the user's game directory.
 *
 * Availability:
 *
 *   alpha 28
 *
 * Status:
 *
 *   experimental
 */
static int l_game_load_game(lua_State *l)
{
	if (Pi::game) {
		luaL_error(l, "can't load a game while a game is already running");
		return 0;
	}

	const std::string filename(luaL_checkstring(l, 1));

	try {
		Pi::game = Game::LoadGame(filename);
	}
	catch (SavedGameCorruptException) {
		luaL_error(l, Lang::GAME_LOAD_CORRUPT);
	}
	catch (SavedGameWrongVersionException) {
		luaL_error(l, Lang::GAME_LOAD_WRONG_VERSION);
	}
	catch (CouldNotOpenFileException) {
		luaL_error(l, Lang::GAME_LOAD_CANNOT_OPEN);
	}

	return 0;
}

/*
 * Function: SaveGame
 *
 * Save the current game.
 *
 * > path = Game.SaveGame(filename)
 *
 * Parameters:
 *
 *   filename - Filename to save to. The file will be placed the 'savefiles'
 *              directory in the user's game directory.
 *
 * Return:
 *
 *   path - the full path to the saved file (so it can be displayed)
 *
 * Availability:
 *
 *   June 2013
 *
 * Status:
 *
 *   experimental
 */
static int l_game_save_game(lua_State *l)
{
	if (!Pi::game) {
		return luaL_error(l, "can't save when no game is running");
	}

	if (Pi::game->IsHyperspace()) {
		return luaL_error(l, "%s", Lang::CANT_SAVE_IN_HYPERSPACE);
	}

	const std::string filename(luaL_checkstring(l, 1));
	const std::string path = FileSystem::JoinPathBelow(Pi::GetSaveDir(), filename);

	try {
		Game::SaveGame(filename, Pi::game);
		lua_pushlstring(l, path.c_str(), path.size());
		return 1;
	}
	catch (CouldNotOpenFileException) {
		const std::string message = stringf(Lang::COULD_NOT_OPEN_FILENAME, formatarg("path", path));
		lua_pushlstring(l, message.c_str(), message.size());
		return lua_error(l);
	}
	catch (CouldNotWriteToFileException) {
		return luaL_error(l, "%s", Lang::GAME_SAVE_CANNOT_WRITE);
	}
}

/*
 * Function: EndGame
 *
 * End the current game and return to the main menu.
 *
 * > Game.EndGame(filename)
 *
 * Availability:
 *
 *   June 2013
 *
 * Status:
 *
 *   experimental
 */
static int l_game_end_game(lua_State *l)
{
	if (Pi::game) {
		Pi::EndGame();
	}
	return 0;
}

/*
 * Attribute: player
 *
 * The <Player> object for the current player.
 *
 * Availability:
 *
 *  alpha 10
 *
 * Status:
 *
 *  stable
 */
static int l_game_attr_player(lua_State *l)
{
	if (!Pi::game)
		lua_pushnil(l);
	else
		LuaObject<Player>::PushToLua(Pi::player);
	return 1;
}

/*
 * Attribute: system
 *
 * The <StarSystem> object for the system the player is currently in.
 *
 * Availability:
 *
 *  alpha 10
 *
 * Status:
 *
 *  stable
 */
static int l_game_attr_system(lua_State *l)
{
	if (!Pi::game)
		lua_pushnil(l);
	else
		LuaObject<StarSystem>::PushToLua(Pi::game->GetSpace()->GetStarSystem().Get());
	return 1;
}

/*
 * Attribute: time
 *
 * The current game time, in seconds since 12:00 01-01-2184
 *
 * Availability:
 *
 *  alpha 10
 *
 * Status:
 *
 *  stable
 */
static int l_game_attr_time(lua_State *l)
{
	if (!Pi::game)
		lua_pushnil(l);
	else
		lua_pushnumber(l, Pi::game->GetTime());
	return 1;
}

// XXX temporary to support StationView "Launch" button
// remove once WorldView has been converted to the new UI
static int l_game_switch_to_world_view(lua_State *l)
{
	if (!Pi::game)
		return luaL_error(l, "can't switch view when no game is running");
	Pi::SetView(Pi::worldView);
	return 0;
}

static int l_game_dev1(lua_State* l)
{
	std::string out;
	if(!Pi::game) {
		out = "Dev1 requires game to be running (Pi::game != nullptr)";
	} else {
		std::list<HyperspaceCloud*> clouds;
		Pi::game->EnumerateAllHyperspaceClouds(clouds);
		if(clouds.size() == 0) {
			out = "No hyperspace clouds found in system.";
		} else {
			std::ostringstream ss;
			int cc = 0;
			int nearest_departure_cloud = -1;
			double cloud_distance = std::numeric_limits<double>::max();

			for(auto hc : clouds) {
				cc += 1;
				ss << "Cloud " << cc << ": ";
				ss << hc->GetCloudTypeString() << " - ";
				ss << (hc->HasShip() ? "Ship" : "Empty") << " - ";
				ss << "due date: " << hc->GetDueDate();
				if(Pi::player) {
					vector3d cp = hc->GetPositionRelTo(Pi::player->GetFrame());
					double dist = (cp - Pi::player->GetPosition()).Length();
					if(dist < cloud_distance) {
						cloud_distance = dist;
						if(hc->SupportsDeparture()) {
							nearest_departure_cloud = cc - 1;	
						}
					}
					ss << " - dist: " << (dist / 1000.0);
				}
				ss << std::endl;
			}
			if(nearest_departure_cloud > -1) {
				ss << "Nearest departure cloud is " << (nearest_departure_cloud + 1) << std::endl;
			} else if(Pi::player) {
				ss << "No departure cloud is found." << std::endl;
			}
			out = ss.str().c_str();
		}
	}
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

static std::string SystemBodyTypeName(SystemBody::BodyType bt) {
	std::string out;
	switch (bt) {
		case SystemBody::BodyType::TYPE_STARPORT_ORBITAL:
			out = "Orbital Starport";
			break;
		case SystemBody::BodyType::TYPE_STARPORT_SURFACE:
			out = "Surface Starport";
			break;
		case SystemBody::BodyType::TYPE_PLANET_ASTEROID:
			out = "Planet Asteroid";
			break;
		case SystemBody::BodyType::TYPE_PLANET_GAS_GIANT:
			out = "Planet Gas Giant";
			break;
		case SystemBody::BodyType::TYPE_PLANET_TERRESTRIAL:
			out = "Planet Terretrial";
			break;
		case SystemBody::BodyType::TYPE_GRAVPOINT:
			out = "Gravity Point";
			break;
        case SystemBody::BodyType::TYPE_HYPERSPACE_CLOUD:
            out = "Hyperspace Cloud";
            break;
		default:
			out = "Star";
			break;
	}
	return out;
}

static int l_game_dev2(lua_State* l)
{
	//std::string out("Not implemented 2");
	//lua_pushlstring(l, out.c_str(), out.size());
	std::string out;
	std::ostringstream ss;
	if (!Pi::game) {
		out = "Dev2 requires game to be running (Pi::game != nullptr)";
	} else {
		ss << "Enumerating all system bodies in current system..." << std::endl;
		auto sbodies = Pi::game->GetSpace()->GetStarSystem()->GetBodiesMap();
		for(auto sb_iter = sbodies.begin(); sb_iter != sbodies.end(); ++sb_iter) {
			ss << "SystemBody[" << sb_iter->first << "] -> Type: ";
			ss << SystemBodyTypeName(sb_iter->second->GetType());
			ss << " " << sb_iter->second->GetName() << " (" << 
				sb_iter->second->GetNumChildren() << " children)";
			if(sb_iter->second->GetNumChildren() > 0) {
				ss << std::endl << "    Children: " << std::endl;
				auto iter_proxy = sb_iter->second->GetChildren();
				for(auto it : iter_proxy) {
					ss << "    > " << SystemBodyTypeName(it->GetType()) << 
						" " << it->GetName() << std::endl;
				}
			}
			ss << std::endl;
			out = ss.str().c_str();
		}
	}
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

static int l_game_dev3(lua_State* l)
{
	//std::string out("Not implemented 3");
	//lua_pushlstring(l, out.c_str(), out.size());
	
	// Destroy a body by index
	/*
	std::string out;
	std::ostringstream ss;
	unsigned sbody_index = luaL_checkunsigned(l, 2);
	if(sbody_index >= Pi::game->GetSpace()->GetStarSystem()->GetNumBodies()) {
		ss << "LuaGame Dev 3 has been given an invalid system body: " << sbody_index << std::endl;
	} else {
		auto sbody_map = Pi::game->GetSpace()->GetStarSystem()->GetBodiesMap();
		Pi::game->GetSpace()->GetStarSystem()->DestroyBody(sbody_map[sbody_index].Get());
	}
	lua_pushlstring(l, out.c_str(), out.size());
	*/

	// Create new system body cloud at player location
	Pi::player->AIFlyToPermaCloud();
	std::string out("Player fly to perma cloud command test.");
    lua_pushlstring(l, out.c_str(), out.size());

	return 1;
}

static int l_game_dev4(lua_State* l)
{
	LuaEvent::Queue("onAICommand", Pi::player, "hyperspawntest");
	std::string out("Sent spawn hyperspace ship.");
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

static int l_game_spawn_test_cargo(lua_State* l)
{
	std::string out;
	if(!Pi::player) {
		out = "Player is null!";
	} else {
		CargoBody* c_body = new CargoBody(Equip::Type::PRECIOUS_METALS);
		bool success = Pi::player->SpawnCargo(c_body);
		if(success) {
			out = "Spawned 1 unit of test cargo";
		} else {
			out = "Failed to spawn test cargo";
		}
	}
	lua_pushlstring(l, out.c_str(), out.size());
	return 1;
}

static int l_game_hyper_spawn_test(lua_State* l)
{
	LuaEvent::Queue("onAICommand", Pi::player, "hyperspawntest");
	return 0;
}

void LuaGame::Register()
{
	lua_State *l = Lua::manager->GetLuaState();

	LUA_DEBUG_START(l);

	static const luaL_Reg l_methods[] = {
		{ "StartGame", l_game_start_game },
		{ "LoadGame",  l_game_load_game  },
		{ "SaveGame",  l_game_save_game  },
		{ "EndGame",   l_game_end_game   },

		{"SwitchToWorldView", l_game_switch_to_world_view},

		{"SpawnTestCargo", l_game_spawn_test_cargo},
		{"SpawnHyperShip", l_game_hyper_spawn_test},

		{ "Dev1", l_game_dev1 },
		{ "Dev2", l_game_dev2 },
		{ "Dev3", l_game_dev3 },
		{ "Dev4", l_game_dev4 },

		{ 0, 0 }
	};

	static const luaL_Reg l_attrs[] = {
		{ "player", l_game_attr_player },
		{ "system", l_game_attr_system },
		{ "time",   l_game_attr_time   },
		{ 0, 0 }
	};

	lua_getfield(l, LUA_REGISTRYINDEX, "CoreImports");
	LuaObjectBase::CreateObject(l_methods, l_attrs, 0);
	lua_setfield(l, -2, "Game");
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);
}
