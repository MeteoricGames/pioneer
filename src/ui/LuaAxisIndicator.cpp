// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "AxisIndicator.h"
#include "LuaObject.h"

namespace UI {

class LuaAxisIndicator {
public:
	static int l_set_axis(lua_State *l) {
		UI::AxisIndicator *axis_index = LuaObject<UI::AxisIndicator>::CheckFromLua(1);
		int aid = luaL_checkinteger(l, 2);
		axis_index->SetAxis(aid);
		return 0;
	}
};

}

using namespace UI;

template <> const char *LuaObject<UI::AxisIndicator>::s_type = "UI.AxisIndicator";

template <> void LuaObject<UI::AxisIndicator>::RegisterClass()
{
	static const char *l_parent = "UI.Label";

	static const luaL_Reg l_methods [] {
		{ "SetAxis", &LuaAxisIndicator::l_set_axis },
		{ 0, 0 }
	};

	LuaObjectBase::CreateClass(s_type, l_parent, l_methods, 0, 0);
	LuaObjectBase::RegisterPromotion(l_parent, s_type, LuaObject<UI::AxisIndicator>::DynamicCastPromotionTest);
}
