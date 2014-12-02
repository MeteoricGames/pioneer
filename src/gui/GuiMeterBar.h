// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef GUIMETERBAR_H
#define GUIMETERBAR_H

#include "Color.h"
#include "GuiLabel.h"
#include "GuiFixed.h"

/* A cute horizontal bar readout of some value from 0 to 1,
   with a text label also. Hull and weapon temperature are shown with these  */
namespace Gui {
	class MeterBar: public Gui::Fixed {
	public:
		enum MeterBarAlign {
			METERBAR_LEFT = 0,
			METERBAR_RIGHT,
		};

		MeterBar(float width, const char *label, const ::Color &graphCol, 
			MeterBarAlign alignment = METERBAR_LEFT, const Color &labelCol = Color::PARAGON_GREEN);
		virtual ~MeterBar() {}
		virtual void Draw();
		virtual void GetSizeRequested(float size[2]);
		void SetValue(float v) { m_barValue = Clamp(v,0.0f,1.0f); }
		void SetColor(const ::Color &c) { m_barColor = c; }
	private:
		float m_requestedWidth;
		Gui::Label *m_label;
		WidgetList::iterator m_labelIter; // Why on earth is the position of label stored in a different object collection?
		::Color m_barColor;
		float m_barValue;
		MeterBarAlign m_alignment;
	};
}

#endif /* GUIMETERBAR_H */
