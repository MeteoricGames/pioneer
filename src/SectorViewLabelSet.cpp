// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Pi.h"
#include "gui/Gui.h"
#include "SectorViewLabelSet.h"

namespace Gui {
	SectorViewLabelSet::SectorViewLabelSet() : Widget()
	{
		m_eventMask = EVENT_MOUSEDOWN;
		m_labelsVisible = true;
		m_labelsClickable = true;
		m_labelColor = Color::WHITE;
		m_font = Screen::GetFont();
		m_tooltipDesc = nullptr;
		m_tooltipEnabled = true;
	}

	bool SectorViewLabelSet::OnMouseDown(Gui::MouseButtonEvent *e)
	{
		if ((e->button == SDL_BUTTON_LEFT) && (m_labelsClickable)) {
			for (std::vector<SectorViewLabelSetItem>::iterator i = m_items.begin(); i != m_items.end(); ++i) {
				if ((fabs(e->x - (*i).screenx) < 10.0f) &&
					(fabs(e->y - (*i).screeny) < 10.0f)) {
					(*i).onClick();
					return false;
				}
			}
		}
		return true;
	}

	void SectorViewLabelSet::UpdateTooltip()
	{
		if (!m_tooltipEnabled 
			|| Pi::MouseButtonState(SDL_BUTTON_LEFT) 
			|| Pi::MouseButtonState(SDL_BUTTON_RIGHT)) 
		{
			HideTooltip();
			return;
		}

		int mouse_pos[2];
		float mouse_coords[2];
		SDL_GetMouseState(&mouse_pos[0], &mouse_pos[1]);
		Screen::SDLEventCoordToScreenCoord(mouse_pos[0], mouse_pos[1], &mouse_coords[0], &mouse_coords[1]);
		bool tooltip_shown = false;
		for(unsigned int i = 0; i < m_items.size(); ++i) {
			if (!m_items[i].name.empty() 
				&& (fabs(mouse_coords[0] - m_items[i].screenx) < 10.0f) 
				&& (fabs(mouse_coords[1] - m_items[i].screeny) < 10.0f)) 
			{
				ShowTooltip(m_items[i].name, m_items[i].type, m_items[i].desc,
					m_items[i].screenx, m_items[i].screeny);
				tooltip_shown = true;
				break;
			}
		}
		if (m_tooltipDesc && !tooltip_shown) {
			HideTooltip();
		}
	}

	void SectorViewLabelSet::ShowTooltip(std::string& name, std::string& type, std::string& desc, int x, int y)
	{
		if (m_tooltipDesc) {
			if (m_tooltipDesc->GetText() == name) {
				return;
			} else {
				HideTooltip();
			}
		}
		std::string str_tooltip = name + "\n" + type + "\n" + desc;
		m_tooltipDesc = new ToolTip(this, str_tooltip);
		float size[2];
		m_tooltipDesc->GetSize(size);
		x = std::max(x - size[0], 0.0f);
		y = std::max(y - size[1], 0.0f);
		if (size[0] + x > Screen::GetWidth()) {
			x = Screen::GetWidth() - size[0];
		}
		if (size[1] + y > Screen::GetHeight()) {
			y = Screen::GetHeight() - size[1];
		}
		Screen::AddBaseWidget(m_tooltipDesc, x, y);
		m_tooltipDesc->SetOutlineColor(Color(Color::PARAGON_BLUE));
		m_tooltipDesc->SetBackgroundColor(Color(8, 8, 16));
		m_tooltipDesc->SetTextColor(Color(Color::PARAGON_GREEN));
		m_tooltipDesc->AddTextLine(type, Color(Color::PARAGON_BLUE));
		m_tooltipDesc->AddTextLine(desc, Color(Color::PARAGON_BLUE));
		m_tooltipDesc->SetText(name);
		m_tooltipDesc->Show();
	}

	void SectorViewLabelSet::HideTooltip()
	{
		if (m_tooltipDesc) {
			Screen::RemoveBaseWidget(m_tooltipDesc);
			delete m_tooltipDesc;
			m_tooltipDesc = 0;
		}
	}

	bool SectorViewLabelSet::CanPutItem(float x, float y)
	{
		for (std::vector<SectorViewLabelSetItem>::iterator i = m_items.begin(); i != m_items.end(); ++i) {
			if ((fabs(x - (*i).screenx) < 5.0f) &&
				(fabs(y - (*i).screeny) < 5.0f)) return false;
		}
		return true;
	}

	void SectorViewLabelSet::Add(sigc::slot<void> onClick, float screenx, float screeny, const Color& col,
		const std::string& text, const std::string& tooltip_name, const std::string& tooltip_type,
		const std::string& tooltip_desc)
	{
		if (CanPutItem(screenx, screeny)) {
			m_items.push_back(
				SectorViewLabelSetItem(onClick, screenx, screeny, col, text, tooltip_name, 
					tooltip_type, tooltip_desc)
			);
		}
	}

	void SectorViewLabelSet::Add(sigc::slot<void> onClick, float screenx, float screeny, const Color& col,
		const std::string& text)
	{
		static const std::string empty_string = "";
		if (CanPutItem(screenx, screeny)) {
			m_items.push_back(
				SectorViewLabelSetItem(onClick, screenx, screeny, col, text, empty_string,
				empty_string, empty_string)
				);
		}
	}

	void SectorViewLabelSet::Clear()
	{
		m_items.clear();
	}

	void SectorViewLabelSet::Draw()
	{
		PROFILE_SCOPED()
			if (!m_labelsVisible) return;
		for (std::vector<SectorViewLabelSetItem>::iterator i = m_items.begin(); i != m_items.end(); ++i) {
			Gui::Screen::RenderString(
				(*i).text, 
				(*i).screenx, 
				(*i).screeny - Gui::Screen::GetFontHeight()*0.5f, 
				(*i).hasOwnColor ? (*i).color : m_labelColor, m_font.Get());
		}
	}

	void SectorViewLabelSet::GetSizeRequested(float size[2])
	{
		size[0] = 800.0f;
		size[1] = 600.0f;
	}

}
