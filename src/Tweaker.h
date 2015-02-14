// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _TWEAKER_H_
#define _TWEAKER_H_

#include "libs.h"
#include "IniConfig.h"

#ifndef TW_CALL
#if (defined(_WIN32) || defined(_WIN64)) && !defined(TW_STATIC)
#   define TW_CALL          __stdcall
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

	static bool Draw();
	static bool TranslateEvent(SDL_Event& event);

	static bool DefineTweak(const std::string& name, std::vector<STweakValue> description);
	static bool Tweak(const std::string& name);
	static bool IsActiveTweak() { return s_activeTweak != nullptr ? true : false; }
	static bool Close();
	static void Enable() { s_enabled = true; }
	static void Disable() { s_enabled = false; }
	static bool IsEnabled() { return s_enabled; }

	static std::string LUAListTweaks();


	static std::map<std::string, std::vector<STweakValue> > s_tweaksMap;
	static std::map<std::string, std::shared_ptr<IniConfig> > s_configMap;

private:
	Tweaker();
	Tweaker(const Tweaker&);
	Tweaker& operator=(const Tweaker&);

	static void TW_CALL callback_CopyToClipboard(void *clientData);
	static void TW_CALL callback_Reset(void *clientData);
	static void TW_CALL callback_Save(void *clientData);

	static bool s_isInit;
	static bool s_enabled;
	static TwBar* s_activeTweak;

	static void UpdateSettings(IniConfig* settings, std::vector<STweakValue>& tweak);
	static void LoadTweak(IniConfig* settings, std::vector<STweakValue>& tweak);
	static TwBar* CreateTweak(std::map<std::string, std::vector<STweakValue> >::iterator& tweak);

	static FileSystem::FileSourceFS* s_gameDataFS;
};


#endif /* _TWEAKER_H */