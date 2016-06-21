// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _TWEAKER_H_
#define _TWEAKER_H_

#include "libs.h"
#include "IniConfig.h"
#include "TweakerMonitors.h"

#ifndef TW_CALL
#if (defined(_WIN32) || defined(_WIN64)) && !defined(TW_STATIC)
#   define TW_CALL __stdcall
#else
#   define TW_CALL
#endif
#endif

union SDL_Event;
struct CTwBar;
typedef struct CTwBar TwBar;

enum ETweakType
{
	ETWEAK_SEPERATOR,
	ETWEAK_FLOAT,
	ETWEAK_INT,
	ETWEAK_COLOR,
	ETWEAK_TOGGLE,
};

struct STweakValue
{
	STweakValue(ETweakType _type, std::string _name, void* _param, void* _default_param, 
		std::string _settings) : type(_type), name(_name), param(_param), 
		settings(_settings), defaultParam(_default_param) {
	}

	STweakValue(ETweakType _type, std::string _name, void* _param, const void* _default_param,
		std::string _settings) : type(_type), name(_name), param(_param),
		settings(_settings), defaultParam(const_cast<void*>(_default_param)) {
	}

	ETweakType type;
	std::string name;
	void* param;
	void* defaultParam;
	std::string settings;
};

class Tweaker
{
public:
	virtual ~Tweaker() {}

	static bool Init(int window_width, int window_height);
	static bool Available() { return s_isInit; }
	static bool Terminate();

	static bool Update();
	static bool Draw();
	static bool TranslateEvent(SDL_Event& event);
	static void NotifyRemoved(const Body* removed_body);

	static bool DefineTweak(const std::string& name, std::vector<STweakValue> description);
	static bool IsTweakDefined(const std::string& name);
	static bool RemoveTweak(const std::string& name);
	static bool Tweak(const std::string& name);
	static bool IsActiveTweak() { return s_activeTweak != nullptr ? true : false; }
	static bool Close();
	static bool Monitor();

	static void Enable() { s_enabled = true; }
	static void Disable() { s_enabled = false; }
	static bool IsEnabled() { return s_enabled; }

	static std::string LUAListTweaks();

private:
	Tweaker();
	Tweaker(const Tweaker&);
	Tweaker& operator=(const Tweaker&);

	static TwBar* CreateMonitor(Body* body);
	static void UpdateMonitor(Body* body);

	static void TW_CALL callback_CopyToClipboard(void *clientData);
	static void TW_CALL callback_Reset(void *clientData);
	static void TW_CALL callback_Save(void *clientData);
	static void TW_CALL callback_AICommand(void *clientData);

	static std::map<std::string, std::vector<STweakValue> > s_tweaksMap;
	static std::map<std::string, std::shared_ptr<IniConfig> > s_configMap;
	//static std::map<Body::Type, TwBar*> s_monitorsMap;

	static bool s_isInit;
	static bool s_enabled;
	static TwBar* s_activeTweak;
	
	// This is a special parameter when true will skip all AntTweakBar specific calls
	// It is to be used for OSX to disable AntTweakBar because it's crashing the game.
	static bool s_doNotUseLib;

	static TM_SHIP s_monitor_ship;
	static TM_HYPERCLOUD s_monitor_cloud;

	static bool s_monitorMode;
	static Body* s_monitorCurrentTarget;
	static TwBar* s_activeMonitor;
	static TW_AICOMMAND s_aiCommand_FlyToPermaCloud;
	static TW_AICOMMAND s_aiCommand_ClearAI;

	static void UpdateSettings(IniConfig* settings, std::vector<STweakValue>& tweak);
	static void LoadTweak(IniConfig* settings, std::vector<STweakValue>& tweak);
	static TwBar* CreateTweak(std::map<std::string, std::vector<STweakValue> >::iterator& tweak);

	static FileSystem::FileSourceFS* s_gameDataFS;
};


#endif /* _TWEAKER_H */