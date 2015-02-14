// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef UI_AXIS_INDICATOR_H
#define UI_AXIS_INDICATOR_H

#include "Widget.h"
#include "Label.h"
#include "SmartPtr.h"

// Visual indicator of a specific joystick axis (works on current joystick only)

namespace UI {

	class AxisIndicator : public Label {
	public:
		virtual void Update();

		void SetAxis(int axis_indicator)
		{
			m_axisIndex = axis_indicator;
		}

	protected:
		friend class Context;
		AxisIndicator(Context *context, int axis_index);

		int m_axisIndex;
	};

}

#endif
