// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GUIBOX_H
#define _GUIBOX_H
/*
 * Box oriented packing widget container.
 */

#include "GuiWidget.h"
#include "GuiContainer.h"

namespace Gui {
	enum BoxOrientation {
		BOX_HORIZONTAL = 0,
		BOX_VERTICAL = 1
	};

	class Box: public Container {
	public:
		Box(BoxOrientation orient);
		void PackStart(Widget *child);
		void PackEnd(Widget *child);
		void Remove(Widget *child);
		virtual ~Box();
		virtual void GetSizeRequested(float size[2]) { GetSizeRequestedOrMinimum(size, false); }
		virtual void GetMinimumSize(float size[2]) { GetSizeRequestedOrMinimum(size, true); }
		virtual void OnChildResizeRequest(Widget *);
		virtual void UpdateAllChildSizes();
		void SetSizeRequest(float size[2]);
		void SetSizeRequest(float x, float y);
		void SetSpacing(float spacing) { m_spacing = spacing; }
		void SetPadding(float padding) { m_padding = padding; }
	private:
		void _Init();
		void GetSizeRequestedOrMinimum(float size[2], bool minimum);
		float m_wantedSize[2];
		float m_spacing;
		float m_padding;
		enum BoxOrientation m_orient;
	protected:
		const float GetSpacing() const { return m_spacing; }
		const float GetPadding() const { return m_padding; }
	};

	class VBox: public Box {
	public:
		VBox() : Box(BOX_VERTICAL) {}
		virtual void GetChildrenSize(float size[2]) override;
	};

	class HBox: public Box {
	public:
		HBox() : Box(BOX_HORIZONTAL) {}
		virtual void GetChildrenSize(float size[2]) override;;
	};
}

#endif /* _GUIBOX_H */

