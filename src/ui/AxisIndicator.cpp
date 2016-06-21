// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "AxisIndicator.h"
#include "Context.h"
#include "Pi.h"
#include <sstream>

namespace UI {

AxisIndicator::AxisIndicator(Context *context, int axis_index) : Label(context, "INPUT: 0"), m_axisIndex(axis_index)
{

}


void AxisIndicator::Update()
{
	if(m_axisIndex < Pi::GetJoystickAxisCount()) {
		std::ostringstream ss;
		ss << "INPUT: " << Pi::ReadJoystickAxisValue(m_axisIndex);
		SetText(ss.str());
	}
}

}