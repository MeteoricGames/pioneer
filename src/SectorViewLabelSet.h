// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef SECTORVIEW_LABELSET_H
#define SECTORVIEW_LABELSET_H

#include "gui/GuiWidget.h"
#include <vector>

/*
* Collection of clickable labels. Used by the WorldView for clickable
* bodies, and SystemView, SectorView etc.
*/
namespace Gui {
	class SectorViewLabelSet : public Widget {
	public:
		class SectorViewLabelSetItem {
		public:
			SectorViewLabelSetItem(sigc::slot<void> onClick_, float screenx_, float screeny_, const Color& c,
				const std::string& _text, const std::string& _name, const std::string& _type,
				const std::string& _desc)
			{
				this->text = _text;
				this->name = _name;
				this->type = _type;
				this->desc = _desc;
				this->onClick = onClick_;
				this->screenx = screenx_;
				this->screeny = screeny_;
				this->color = c;
				this->hasOwnColor = false;
			}
			std::string text;
			std::string name;
			std::string type;
			std::string desc;
			Color color;
			bool hasOwnColor;
			sigc::slot<void> onClick;
			float screenx, screeny;
		};

		SectorViewLabelSet();
		bool OnMouseDown(MouseButtonEvent *e);
		void UpdateTooltip();
		virtual void Draw();
		virtual void GetSizeRequested(float size[2]);
		void Clear();
		void Add(sigc::slot<void> onClick, float screenx, float screeny, const Color& col,
			const std::string& text, const std::string& tooltip_name, const std::string& tooltip_type,
			const std::string& tooltip_desc);
		void Add(sigc::slot<void> onClick, float screenx, float screeny, const Color& col,
			const std::string& text);
		void SetLabelsClickable(bool v) { m_labelsClickable = v; }
		void SetLabelsVisible(bool v) { m_labelsVisible = v; }
		void SetLabelColor(const Color &c) { m_labelColor = c; }
		void DisableTooltip() { m_tooltipEnabled = false; }
		void EnableTooltip() { m_tooltipEnabled = true; }

	private:
		bool CanPutItem(float x, float y);
		void ShowTooltip(std::string& name, std::string& type, std::string& desc, int x, int y);
		void HideTooltip();

		std::vector<SectorViewLabelSetItem> m_items;
		bool m_labelsVisible;
		bool m_labelsClickable;
		bool m_tooltipEnabled;
		Color m_labelColor;
		ToolTip* m_tooltipDesc;

		RefCountedPtr<Text::TextureFont> m_font;
	};
}

#endif /* SECTORVIEW_LABELSET_H */
