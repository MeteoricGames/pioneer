// Copyright � 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Tweaker.h"
#include "TweakerMonitors.h"
#include <SDL.h>
#include "graphics/GraphicsHardware.h"
#include <sstream>
#include "FileSystem.h"
#include "Pi.h"
#include "Game.h"
#include "Player.h"
#include "LuaEvent.h"

#include <AntTweakBar.h>
#if defined(_WIN32) || defined(__linux) || defined(__unix)
bool Tweaker::s_doNotUseLib = false;
#define TWEAKER(call) call
#elif defined(__APPLE__)
bool Tweaker::s_doNotUseLib = true;
#define TWEAKER(call)
#endif

bool Tweaker::s_isInit = false;
bool Tweaker::s_enabled = true;

bool Tweaker::s_monitorMode = false;
Body* Tweaker::s_monitorCurrentTarget = nullptr;
TwBar* Tweaker::s_activeMonitor = nullptr;
TM_SHIP Tweaker::s_monitor_ship;
TM_HYPERCLOUD Tweaker::s_monitor_cloud;

std::map<std::string, std::vector<STweakValue> > Tweaker::s_tweaksMap;
std::map<std::string, std::shared_ptr<IniConfig> > Tweaker::s_configMap;
TwBar* Tweaker::s_activeTweak = nullptr;
TW_AICOMMAND Tweaker::s_aiCommand_FlyToPermaCloud;
TW_AICOMMAND Tweaker::s_aiCommand_ClearAI;

FileSystem::FileSourceFS* Tweaker::s_gameDataFS = nullptr;

bool Tweaker::Init(int window_width, int window_height)
{
	if(s_isInit) {
		return true;
	}
	s_gameDataFS = new FileSystem::FileSourceFS(FileSystem::GetDataDir(), true);
    
    if(s_doNotUseLib) {
        s_isInit = true;
        return true;
    }
    
	int success;
	if(Graphics::Hardware::context_CoreProfile) {
		TWEAKER(success = TwInit(TW_OPENGL_CORE, nullptr));
	} else {
		TWEAKER(success = TwInit(TW_OPENGL, nullptr));
	}
	assert(success);
	if(success) {
		s_isInit = true;
		TwWindowSize(window_width, window_height);
		return true; 
	} else {
#ifndef NDEBUG
		std::ostringstream ss;
		const char* last_error = TwGetLastError();
		if(last_error) {
			ss << "Could not initialize AntTweakBar: " << last_error;
		} else {
			ss << "Could not initialize AntTweakBar";
		}
		throw new std::runtime_error(ss.str().c_str());
#endif
		return false;
	}
}

bool Tweaker::Terminate()
{
	if(s_isInit && !s_doNotUseLib) {
		Tweaker::Close();
		return TwTerminate() == 0? false : true;
	} else {
		return false;
	}
}

bool Tweaker::Update()
{
	if(s_isInit && s_enabled) {
		if(s_monitorMode && Pi::player) {
			Body* nt = Pi::player->GetNavTarget();
			Body* ct = Pi::player->GetCombatTarget();
			//Body* target = nt != nullptr ? nt : ct; // Prioritize navtarget over combat target
			Body* target = ct != nullptr ? ct : nt; // Prioritize combattarget since we only care about ships now
			// Check if target changed
			if(target != s_monitorCurrentTarget) {
				// Target changed.
				// Remove active monitor if necessary
				if( s_activeMonitor && target && s_monitorCurrentTarget &&
				    target->GetType() == s_monitorCurrentTarget->GetType()) 
				{
					// Keep the same monitor
				} else {
					// Change monitor
					TwDeleteBar(s_activeMonitor);
					s_activeMonitor = nullptr;
					if(target) { // Only add new monitor if there is a valid target
						s_activeMonitor = CreateMonitor(target);
					}
				}
				s_monitorCurrentTarget = target;
			}
			// Updated monitors
			UpdateMonitor(target);
		}
		return true;
	} else {
		return false;
	}
}

bool Tweaker::Draw()
{
	if(s_isInit && s_enabled && !s_doNotUseLib) {
		return TwDraw() == 0 ? false : true;
	} else {
		return false;
	}
}

bool Tweaker::TranslateEvent(SDL_Event& event)
{
	if(!s_isInit || !s_enabled || s_doNotUseLib) {
		return false;
	}

	int handled = 0;
	switch (event.type)
	{
	case SDL_KEYDOWN:
		if (event.key.keysym.sym & SDLK_SCANCODE_MASK) {
			int key = 0;
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				key = TW_KEY_UP;
				break;
			case SDLK_DOWN:
				key = TW_KEY_DOWN;
				break;
			case SDLK_RIGHT:
				key = TW_KEY_RIGHT;
				break;
			case SDLK_LEFT:
				key = TW_KEY_LEFT;
				break;
			case SDLK_INSERT:
				key = TW_KEY_INSERT;
				break;
			case SDLK_HOME:
				key = TW_KEY_HOME;
				break;
			case SDLK_END:
				key = TW_KEY_END;
				break;
			case SDLK_PAGEUP:
				key = TW_KEY_PAGE_UP;
				break;
			case SDLK_PAGEDOWN:
				key = TW_KEY_PAGE_DOWN;
				break;
			default:
				if (event.key.keysym.sym >= SDLK_F1 &&
					event.key.keysym.sym <= SDLK_F12) {
					key = event.key.keysym.sym + TW_KEY_F1 - SDLK_F1;
				}
				break;
			}
			if (key != 0) {
				handled = TwKeyPressed(key, event.key.keysym.mod);
			}
		} else {
			handled = TwKeyPressed(event.key.keysym.sym /*& 0xFF*/,
				event.key.keysym.mod);
		}
		break;
	case SDL_MOUSEMOTION:
		handled = TwMouseMotion(event.motion.x, event.motion.y);
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		if (event.type == SDL_MOUSEBUTTONDOWN &&
			(event.button.button == 4 || event.button.button == 5)) {
			// mouse wheel
			static int s_WheelPos = 0;
			if (event.button.button == 4)
				++s_WheelPos;
			else
				--s_WheelPos;
			handled = TwMouseWheel(s_WheelPos);
		} else {
			handled = TwMouseButton(
				(event.type == SDL_MOUSEBUTTONUP) ?
			TW_MOUSE_RELEASED : TW_MOUSE_PRESSED,
								(TwMouseButtonID)event.button.button);
		}
		break;
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
			// tell the new size to TweakBar
			TwWindowSize(event.window.data1, event.window.data2);
			// do not set 'handled'
			// SDL_VIDEORESIZE may be also processed by the calling application
		}
		break;
	}

	return handled != 0? true : false;
}

void Tweaker::NotifyRemoved(const Body* removed_body)
{
	if (!s_isInit || !s_enabled) {
		return;
	}
	if(removed_body && s_monitorCurrentTarget == removed_body) {
		s_monitorCurrentTarget = nullptr;
		if(s_activeMonitor) {
			TwDeleteBar(s_activeMonitor);
			s_activeMonitor = nullptr;
		}
	}
}

bool Tweaker::DefineTweak(const std::string& name, std::vector<STweakValue> description)
{
	assert(!name.empty() && description.size() > 0);
	if(name.empty() || description.size() == 0) {
		return false;
	}
	auto search_it = s_tweaksMap.find(name);
	if(search_it != s_tweaksMap.end()) {
		// Tweak already defined
		assert(0);
		return false;
	}  else {
		s_tweaksMap.insert(std::make_pair(name, description));
		// Read ./features/{name} settings file. 
		// Create one if none found using *param as data (loaded with defaults).
		if (description.size() > 0) {
			auto cit = s_configMap.find(name);
			if(cit == s_configMap.end()) {
				// Create new config
				IniConfig* cfg = new IniConfig();
				std::string features_path = "features/" + name;
				if(cfg->Read(*s_gameDataFS, features_path)) {
					// Found and read a file, fetch the values
					LoadTweak(cfg, description);
				} else {
					// No file found. Use default values no need to create a settings file! (optimization)
				}
				s_configMap.insert(std::make_pair(name, std::shared_ptr<IniConfig>(cfg)));
			} else {
				// Found already defined config
			}
		}
		return true;
	}
	
}

bool Tweaker::IsTweakDefined(const std::string& name)
{
	auto search_it = s_tweaksMap.find(name);
	if(search_it == s_tweaksMap.end()) {
		return false;
	} else {
		return true;
	}
}

bool Tweaker::RemoveTweak(const std::string& name)
{
	assert(!name.empty());
	if(name.empty()) {
		return false;
	}
	auto search_it = s_tweaksMap.find(name);
	if(search_it != s_tweaksMap.end()) {
		// tweak found
		if(s_activeTweak) {
			Tweaker::Close();
			s_activeTweak = nullptr;
		}
		s_tweaksMap.erase(search_it);
		return true;
	} else {
		// tweak not found
		return false;
	}
}

bool Tweaker::Tweak(const std::string& name) 
{
	assert(!name.empty());
	auto search_it = s_tweaksMap.find(name);
	if(search_it != s_tweaksMap.end()) {
		// Tweak found! Create bar
		CreateTweak(search_it);
		return true;
	} else {
		// Tweak not found
		return false;
	}
}

void Tweaker::LoadTweak(IniConfig* settings, std::vector<STweakValue>& tweak)
{
	assert(settings);
	for(unsigned i = 0; i < tweak.size(); ++i) {
		switch(tweak[i].type) {
			case ETWEAK_FLOAT: {
				float* v = static_cast<float*>(tweak[i].param);
				*v = settings->Float("", tweak[i].name, *v);
				}
				break;

			case ETWEAK_INT: {
				int* v = static_cast<int*>(tweak[i].param);
				*v = settings->Int("", tweak[i].name, *v);
				}
				break;

			case ETWEAK_COLOR: {
				Color4f* v = static_cast<Color4f*>(tweak[i].param);
				if(settings->HasEntry(tweak[i].name)) {
					// Decode and set
					std::istringstream ss(settings->String(tweak[i].name));
					ss >> v->r >> v->g >> v->b >> v->a;
				} else {
					// Use default value of tweak if no entry is defined
					std::ostringstream ss;
					ss << v->r << " " << v->g << " " << v->b << " " << v->a;
					settings->SetString(tweak[i].name, ss.str());
				}
				}
				break;

			case ETWEAK_SEPERATOR:
				break;

			case ETWEAK_TOGGLE: {
				bool* v = static_cast<bool*>(tweak[i].param);
				*v = settings->Int("", tweak[i].name, *v ? 1 : 0) ? true : false;
				}
				break;

			default:
				continue;
		}
	}
}

void Tweaker::UpdateSettings(IniConfig* settings, std::vector<STweakValue>& tweak)
{
	assert(settings);
	for (auto& tv : tweak) {
		switch (tv.type) {
		case ETWEAK_FLOAT:
			settings->SetFloat(tv.name, *static_cast<float*>(tv.param));
			break;

		case ETWEAK_INT:
			settings->SetInt(tv.name, *static_cast<int*>(tv.param));
			break;

		case ETWEAK_COLOR: {
				std::ostringstream ss;
				Color4f* c = static_cast<Color4f*>(tv.param);
				ss << c->r << " " << c->g << " " << c->b << " " << c->a;
				settings->SetString(tv.name, ss.str());
			}
			break;

		case ETWEAK_SEPERATOR:
			break;

		case ETWEAK_TOGGLE:
				settings->SetInt(tv.name, *static_cast<int*>(tv.param));
			break;

		default:
			continue;
		}
	}
}

void TW_CALL Tweaker::callback_CopyToClipboard(void *clientData)
{
	const char* name = reinterpret_cast<const char*>(clientData);
	auto tweak_desc = Tweaker::s_tweaksMap.find(name);
	if (tweak_desc == Tweaker::s_tweaksMap.end()) {
		assert(0);
		return;
	}
	std::ostringstream ss;
	ss << tweak_desc->first.c_str() << std::endl;
	for(auto& tv : tweak_desc->second) {
		if(tv.type == ETWEAK_SEPERATOR) {
			continue;
		}
		ss << tv.name << "=";
		switch(tv.type) {
			case ETWEAK_FLOAT:
				ss << (*(static_cast<float*>(tv.param)));
				break;
				
			case ETWEAK_INT:
				ss << (*(static_cast<int*>(tv.param)));
				break;

			case ETWEAK_TOGGLE:
				ss << (*(static_cast<bool*>(tv.param)));
				break;

			case ETWEAK_COLOR: {
					float* c = static_cast<float*>(tv.param);
					ss << "RGBA(" << c[0] << ", " << c[1] << ", " << c[2] << ", " << c[3] << ")";
				}
				break;

			default:
				continue;
		}
		ss << std::endl;
	}
	SDL_SetClipboardText(ss.str().c_str());
}

void TW_CALL Tweaker::callback_Reset(void *clientData)
{
	// Reset values to default defined in TweakerSettings.h
	const char* name = reinterpret_cast<const char*>(clientData);
	auto tweak_desc = Tweaker::s_tweaksMap.find(name);
	if (tweak_desc == Tweaker::s_tweaksMap.end()) {
		assert(0);
		return;
	}
	for (auto& tv : tweak_desc->second) {
		switch (tv.type) {
			case ETWEAK_FLOAT:
				*(static_cast<float*>(tv.param)) = *(static_cast<float*>(tv.defaultParam));
				break;

			case ETWEAK_INT:
				*(static_cast<int*>(tv.param)) = *(static_cast<int*>(tv.defaultParam));
				break;

			case ETWEAK_TOGGLE:
				*(static_cast<bool*>(tv.param)) = *(static_cast<bool*>(tv.defaultParam));
				break;

			case ETWEAK_COLOR:
				*(static_cast<Color4f*>(tv.param)) = *(static_cast<Color4f*>(tv.defaultParam));	
				break;

			default:
				continue;
		}
	}
}

void TW_CALL Tweaker::callback_Save(void *clientData)
{
	// Save tweaker values to persistent file
	const char* name = reinterpret_cast<const char*>(clientData);
	auto cfg_it = Tweaker::s_configMap.find(name);
	IniConfig* cfg = cfg_it->second.get();
	if(cfg && name) {
		auto tweak_it = Tweaker::s_tweaksMap.find(name);
		if(tweak_it != Tweaker::s_tweaksMap.end()) {
			UpdateSettings(cfg, tweak_it->second);
		}
	}
	std::string features_path = "features/" + cfg_it->first;
	cfg->Write(*s_gameDataFS, features_path);
}

void TW_CALL Tweaker::callback_AICommand(void *clientData)
{
	TW_AICOMMAND* cmd = static_cast<TW_AICOMMAND*>(clientData);
	LuaEvent::Queue("onAICommand", cmd->ship, cmd->command.c_str());
}

TwBar* Tweaker::CreateTweak(std::map<std::string, std::vector<STweakValue> >::iterator& tweak)
{
	if (s_activeTweak != nullptr) {
		Tweaker::Close();
	}
    TwBar* bar = nullptr;
	TWEAKER(bar = TwNewBar(tweak->first.c_str()));
	auto cfg_it = s_configMap.find(tweak->first);
	IniConfig* cfg = cfg_it != s_configMap.end()? cfg_it->second.get() : nullptr;
	for(auto& tv : tweak->second) {
		switch(tv.type) {
			case ETWEAK_FLOAT:
				TWEAKER(TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_FLOAT, tv.param, tv.settings.c_str()));
				if(cfg) {
					cfg->SetFloat(tv.name, *static_cast<float*>(tv.param));
				}
				break;

			case ETWEAK_INT:
				TWEAKER(TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_INT32, tv.param, tv.settings.c_str()));
				if(cfg) {
					cfg->SetInt(tv.name, *static_cast<int*>(tv.param));
				}
				break;

			case ETWEAK_COLOR:
				TWEAKER(TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_COLOR4F, tv.param, tv.settings.c_str()));
				if(cfg) {
					std::ostringstream ss;
					Color4f* c = static_cast<Color4f*>(tv.param);
					ss << c->r << " " << c->g << " " << c->b << " " << c->a;
					cfg->SetString(tv.name, ss.str());
				}
				break;

			case ETWEAK_SEPERATOR:
				TWEAKER(TwAddSeparator(bar, tv.name.c_str(), tv.settings.c_str()));
				break;

			case ETWEAK_TOGGLE:
				TWEAKER(TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_BOOL32, tv.param, tv.settings.c_str()));
				if(cfg) {
					cfg->SetInt(tv.name, *static_cast<int*>(tv.param));
				}
				break;

			default:
				continue;
		}
	}
	
	TWEAKER(TwAddSeparator(bar, "data", nullptr));
	// Add copy to clipboard button
	TWEAKER(TwAddButton(bar, "CopyToClipboard", callback_CopyToClipboard,
		(void*)(tweak->first.c_str()), "label='To Clipboard'"));
	// Add reset button
	TWEAKER(TwAddButton(bar, "Reset", callback_Reset,
		(void*)(tweak->first.c_str()), "label='Reset'"));
	// Add save button
	TWEAKER(TwAddButton(bar, "Save", callback_Save,
		(void*)(tweak->first.c_str()), "label='Save'"));
	//
	s_activeTweak = bar;
	return bar;
}

bool Tweaker::Close()
{
	TWEAKER(TwDeleteAllBars());
	s_activeTweak = nullptr;
	s_monitorCurrentTarget = nullptr;
	s_activeMonitor = nullptr;
	return true;
}

bool Tweaker::Monitor()
{
	s_monitorMode = true;
	s_monitorCurrentTarget = nullptr;
	s_activeMonitor = nullptr;
	return true;
}

std::string Tweaker::LUAListTweaks()
{
	if(!s_isInit || s_doNotUseLib) {
		return "Tweaker is not initialized.";
	} else {
		if(s_tweaksMap.size() == 0) {
			return "No tweaks are defined.";
		} else {
			std::ostringstream ss;
			for(auto& it : s_tweaksMap) {
				if(it.first[0] != '_') {
					ss << it.first << " ";
				}
			}
			return ss.str();
		}
	}
}

TwBar* Tweaker::CreateMonitor(Body* body)
{
	TwBar* bar = nullptr;

	std::ostringstream ss;
	const int mw = static_cast<int>(Pi::renderer->GetWindow()->GetWidth() * 0.8f);
	const int mh = static_cast<int>(Pi::renderer->GetWindow()->GetHeight() * 0.25);
	const int vw = static_cast<int>(mw * 0.88f);
	const int px = Pi::renderer->GetWindow()->GetWidth() - mw - 16;
	ss << " Main label='~ " << body->GetTypeString() << " ~' ";
	ss << "position='" << px << " " << 16 << "' ";
	ss << "size='" << mw << " " << mh << "' ";
	ss << "valueswidth=" << vw << " ";
	ss << "refresh=" << 0.33 << " ";
	bar = TwNewBar("Main");
	TwDefine(ss.str().c_str());

	switch(body->GetType()) {
		case Body::Type::SHIP:
			// Ship monitor
			{
				Ship* s = static_cast<Ship*>(body);
				TwAddVarRO(bar, "Name", TW_TYPE_STDSTRING, &s_monitor_ship.Name, nullptr);
				TwAddVarRO(bar, "Velocity", TW_TYPE_DOUBLE, &s_monitor_ship.Velocity, nullptr);
				TwAddVarRO(bar, "Direction", TW_TYPE_DIR3D, &s_monitor_ship.Direction.x, nullptr);
				TwAddVarRO(bar, "AIStatus", TW_TYPE_STDSTRING, &s_monitor_ship.AIStatus, nullptr);
				TwAddVarRO(bar, "ModuleName", TW_TYPE_STDSTRING, &s_monitor_ship.ModuleName, nullptr);
				TwAddVarRO(bar, "ModuleStatus", TW_TYPE_STDSTRING, &s_monitor_ship.ModuleStatus, nullptr);
				s_aiCommand_FlyToPermaCloud.command = AICOMMAND_FLYTO_PERMACLOUD;
				s_aiCommand_FlyToPermaCloud.ship = s;
				TwAddButton(bar, "FlyToPermaCloud", callback_AICommand, &s_aiCommand_FlyToPermaCloud,
					" label='Command: Fly To Permanent Cloud' color='255 0 0' ");
				s_aiCommand_ClearAI.command = AICOMMAND_CLEARAI;
				s_aiCommand_ClearAI.ship = s;
				TwAddButton(bar, "ClearAI", callback_AICommand, &s_aiCommand_ClearAI,
					" label='Command: Clear AI' color='255 0 0' ");
			}
			break;

		case Body::Type::HYPERSPACECLOUD:
			{
				TwAddVarRO(bar, "Type", TW_TYPE_STDSTRING, &s_monitor_cloud.Type, nullptr);
				TwAddVarRO(bar, "HasShip", TW_TYPE_BOOLCPP, &s_monitor_cloud.HasShip, nullptr);
				TwAddVarRO(bar, "ShipName", TW_TYPE_STDSTRING, &s_monitor_cloud.ShipName, nullptr);
				TwAddVarRO(bar, "Due", TW_TYPE_DOUBLE, &s_monitor_cloud.Due, nullptr);
				TwAddVarRO(bar, "GameTime", TW_TYPE_DOUBLE, &s_monitor_cloud.GameTime, nullptr);
			}
			break;

		default:
			// Object monitor (unknown)
			{
				TwAddButton(bar, "info.1", nullptr, nullptr, " label='Monitor not defined' ");
				TwAddButton(bar, "info.2", nullptr, nullptr, " label='for this type.' ");
			}
	}
	return bar;
}

void Tweaker::UpdateMonitor(Body* body)
{
	static char buffer[256];
	if (body) {
		switch (body->GetType()) {
			case Body::Type::SHIP:
				{
					Ship* s = static_cast<Ship*>(body);
					s_monitor_ship.Name = s->GetLabel();
					vector3d vel = s->GetVelocity();
					vector3d dir = s->GetOrient().VectorZ();
					s_monitor_ship.Velocity = vel.Length();
					s_monitor_ship.Direction = dir;
					s->AIGetStatusText(buffer);
					s_monitor_ship.AIStatus = buffer;
					s_monitor_ship.ModuleName = s->GetModuleName();
					s_monitor_ship.ModuleStatus = s->GetModuleStatus();
				}
				break;

			case Body::Type::HYPERSPACECLOUD:
				{
					HyperspaceCloud* hc = static_cast<HyperspaceCloud*>(body);
					s_monitor_cloud.HasShip = hc->HasShip();
					s_monitor_cloud.Due = hc->GetDueDate();
					s_monitor_cloud.GameTime = Pi::game->GetTime();
					s_monitor_cloud.Type = hc->GetCloudTypeString();
					if(hc->HasShip()) {
						s_monitor_cloud.ShipName = hc->GetShip()->GetLabel();
					} else {
						s_monitor_cloud.ShipName = "Empty";
					}
				}
				break;

			default:
				// Not implemented
				break;
		}
	}
}