// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Gui.h"

static const float METERBAR_PADDING		= 4.0f;
static const float METERBAR_BAR_HEIGHT	= 8.0f;
static const float LABEL_WIDTH			= 32.0f;

namespace Gui {

MeterBar::MeterBar(float width, const char *label, const ::Color &graphCol, MeterBarAlign alignment,
	const Color &labelCol)
{
	m_requestedWidth = width;
	m_barValue = 0;
	m_barColor = graphCol;
	m_alignment = alignment;
	Gui::Screen::PushFont("HudFont");
	m_label = new Gui::Label(label);
	m_label->Color(labelCol);

	if (m_alignment == METERBAR_LEFT) {
		Add(m_label, m_requestedWidth - LABEL_WIDTH, 0.0f);
	} else {
		Add(m_label, 0.0f, -1.0f);
	}
	m_label->Show();
	m_labelIter = std::find_if(m_children.begin(), m_children.end(), 
		[this](const Gui::Container::widget_pos& wi)->bool{ return wi.w == m_label; });
	assert(m_labelIter != m_children.end());
	Gui::Screen::PopFont();
}

void MeterBar::Draw()
{
	PROFILE_SCOPED()
	float size[2], sizeback[2];
	GetSize(size);

	Graphics::Renderer *r = Gui::Screen::GetRenderer();

	Graphics::Renderer::MatrixTicket ticket(r, Graphics::MatrixMode::MODELVIEW);

	if (m_alignment == METERBAR_LEFT) {
		r->Translate(METERBAR_PADDING, METERBAR_PADDING, 0.0f);
	} else {
		r->Translate(METERBAR_PADDING + LABEL_WIDTH, METERBAR_PADDING, 0.0f);
		vector2f s;
		m_label->GetSize(&s.x);
		m_labelIter->pos[0] = LABEL_WIDTH - s.x - 4.0f;
	}

	sizeback[0] = (size[0] - LABEL_WIDTH) - 2.0f * METERBAR_PADDING;
	sizeback[1] = METERBAR_BAR_HEIGHT;
	size[0] = m_barValue * sizeback[0];
	size[1] = METERBAR_BAR_HEIGHT;
	Gui::Theme::DrawRoundEdgedRect(sizeback, 3.0f, Color(255, 255, 255, 32), Screen::alphaBlendState);
	Gui::Theme::DrawRoundEdgedRect(size, 3.0f, m_barColor, Screen::alphaBlendState);

	if (m_alignment == METERBAR_RIGHT) {
		r->Translate(-LABEL_WIDTH, 0.0f, 0.0f);
	}

	Gui::Fixed::Draw();
}

void MeterBar::GetSizeRequested(float size[2])
{
	size[0] = m_requestedWidth;
	size[1] = METERBAR_PADDING * 2.0f + METERBAR_BAR_HEIGHT + Gui::Screen::GetFontHeight();
}

}
