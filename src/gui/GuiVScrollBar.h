// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GUIVSCROLLBAR
#define _GUIVSCROLLBAR

#include "GuiWidget.h"

namespace Gui {
	class ScrollBar: public Widget {
	public:
		ScrollBar(bool isHoriz);
		virtual ~ScrollBar();
		virtual bool OnMouseDown(MouseButtonEvent *e);
		virtual void GetSizeRequested(float size[2]);
		virtual void GetMinimumSize(float size[2]);
		virtual void Draw();
		void SetAdjustment(Adjustment *adj) {
			m_adjustment = adj;
		}
	protected:
		Adjustment *m_adjustment;
	private:
		void OnRawMouseUp(MouseButtonEvent *e);
		void OnRawMouseMotion(MouseMotionEvent *e);
		bool m_isPressed, m_isHoriz;
		sigc::connection _m_release, _m_motion;
	};

	class VScrollBar: public ScrollBar {
	public:
		VScrollBar(): ScrollBar(false) {}
	};

	class NavVScrollBar : public VScrollBar {
	public:
		NavVScrollBar();
		virtual ~NavVScrollBar();
		virtual void Draw();
		void SetOffsetY(float offset);
		const float GetOffsetY() const;
	private:
		virtual void LoadImages();
		Image *m_topImage, *m_midImage, *m_bottomImage, *m_thumbImage;
		float m_offsetY = 0.0f; // This shouldnt be needed, I'll look into it again.
	};
}

#endif /* _GUIVSCROLLBAR */
