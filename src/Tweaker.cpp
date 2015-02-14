// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Tweaker.h"
#include <SDL.h>
#include <AntTweakBar.h>
#include "graphics/GraphicsHardware.h"
#include <sstream>
#include "FileSystem.h"

bool Tweaker::s_isInit = false;
bool Tweaker::s_enabled = true;
std::map<std::string, std::vector<STweakValue> > Tweaker::s_tweaksMap;
std::map<std::string, std::shared_ptr<IniConfig> > Tweaker::s_configMap;
TwBar* Tweaker::s_activeTweak = nullptr;

FileSystem::FileSourceFS* Tweaker::s_gameDataFS = nullptr;

bool Tweaker::Init(int window_width, int window_height)
{
	if(s_isInit) {
		return true;
	}
	s_gameDataFS = new FileSystem::FileSourceFS(FileSystem::GetDataDir(), true);
	int success;
	if(Graphics::Hardware::context_CoreProfile) {
		success = TwInit(TW_OPENGL_CORE, nullptr);
	} else {
		success = TwInit(TW_OPENGL, nullptr);
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
	if(s_isInit) {
		return TwTerminate() == 0? false : true;
	} else {
		return false;
	}
}

bool Tweaker::Draw()
{
	if(s_isInit && s_enabled) {
		return TwDraw() == 0 ? false : true;
	} else {
		return false;
	}
}

bool Tweaker::TranslateEvent(SDL_Event& event)
{
	if(!s_isInit || !s_enabled) {
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
		// Read ./features/{name} settings file. Create one if none using *param as data (loaded with defaults).
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

TwBar* Tweaker::CreateTweak(std::map<std::string, std::vector<STweakValue> >::iterator& tweak)
{
	if (s_activeTweak != nullptr) {
		Tweaker::Close();
	}
	TwBar* bar = TwNewBar(tweak->first.c_str());
	auto cfg_it = s_configMap.find(tweak->first);
	IniConfig* cfg = cfg_it != s_configMap.end()? cfg_it->second.get() : nullptr;
	for(auto& tv : tweak->second) {
		switch(tv.type) {
			case ETWEAK_FLOAT:
				TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_FLOAT, tv.param, tv.settings.c_str());
				if(cfg) {
					cfg->SetFloat(tv.name, *static_cast<float*>(tv.param));
				}
				break;

			case ETWEAK_INT:
				TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_INT32, tv.param, tv.settings.c_str());
				if(cfg) {
					cfg->SetInt(tv.name, *static_cast<int*>(tv.param));
				}
				break;

			case ETWEAK_COLOR:
				TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_COLOR4F, tv.param, tv.settings.c_str());
				if(cfg) {
					std::ostringstream ss;
					Color4f* c = static_cast<Color4f*>(tv.param);
					ss << c->r << " " << c->g << " " << c->b << " " << c->a;
					cfg->SetString(tv.name, ss.str());
				}
				break;

			case ETWEAK_SEPERATOR:
				TwAddSeparator(bar, tv.name.c_str(), tv.settings.c_str());
				break;

			case ETWEAK_TOGGLE:
				TwAddVarRW(bar, tv.name.c_str(), TW_TYPE_BOOL32, tv.param, tv.settings.c_str());
				if(cfg) {
					cfg->SetInt(tv.name, *static_cast<int*>(tv.param));
				}
				break;

			default:
				continue;
		}
	}
	
	TwAddSeparator(bar, "data", nullptr);
	// Add copy to clipboard button
	TwAddButton(bar, "CopyToClipboard", callback_CopyToClipboard, 
		(void*)(tweak->first.c_str()), "label='To Clipboard'");
	// Add reset button
	TwAddButton(bar, "Reset", callback_Reset,
		(void*)(tweak->first.c_str()), "label='Reset'");
	// Add save button
	TwAddButton(bar, "Save", callback_Save,
		(void*)(tweak->first.c_str()), "label='Save'");
	//
	s_activeTweak = bar;
	return bar;
}

bool Tweaker::Close()
{
	TwDeleteAllBars();
	s_activeTweak = nullptr;
	return true;
}

std::string Tweaker::LUAListTweaks()
{
	if(!s_isInit) {
		return "Tweaker is not initialized.";
	} else {
		if(s_tweaksMap.size() == 0) {
			return "No tweaks are defined.";
		} else {
			std::ostringstream ss;
			for(auto& it : s_tweaksMap) {
				ss << it.first << " ";
			}
			return ss.str();
		}
	}
}