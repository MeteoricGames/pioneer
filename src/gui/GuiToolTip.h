// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GUITOOLTIP_H
#define _GUITOOLTIP_H

#include "GuiWidget.h"
#include <string>

namespace Gui {
	class ToolTip: public Widget {
	public:
		ToolTip(Widget *owner, const char *text);
		ToolTip(Widget *owner, std::string &text);
		virtual void Draw();
		virtual ~ToolTip();
		virtual void GetSizeRequested(float size[2]);
		void SetText(const char *text);
		void SetText(std::string &text);
		void SetTextColor(Color color) { m_textColors[0] = color; }
		void SetBackgroundColor(Color color) { m_bgColor = color; }
		void SetOutlineColor(Color color) { m_outlineColor = color; }
		
		//void SetTextColor(Color4ub& color) { m_textColors[0] = color; }
		//void SetBackgroundColor(Color4ub &color) { m_bgColor = color; }
		//void SetOutlineColor(Color4ub &color) { m_outlineColor = color; }
		
		const std::string& GetText() const { return m_textLines[0]; }

		// Multiple lines
		void AddTextLine(std::string& text, Color color = Color::WHITE);

	private:
		void Init();
		void CalcSize();
		Widget *m_owner;
		std::vector<std::string> m_textLines;
		std::vector<TextLayout*> m_textLayouts;
		std::vector<Color> m_textColors;
		std::vector<float> m_textHeights;
		Color m_bgColor;
		Color m_outlineColor;
		Uint32 m_createdTime;
	};
}

#endif /* _GUITOOLTIP_H */
