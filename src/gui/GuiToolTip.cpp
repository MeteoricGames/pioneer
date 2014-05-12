// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Gui.h"

namespace Gui {

static const float TOOLTIP_PADDING = 5.f;
static const float FADE_TIME_MS	   = 500.f;

ToolTip::ToolTip(Widget *owner, const char *text)
{
	m_owner = owner;
	Init();
	SetText(text);
}

ToolTip::ToolTip(Widget *owner, std::string &text)
{
	m_owner = owner;
	Init();
	SetText(text.c_str());
}

void ToolTip::Init()
{
	m_textLines.push_back("");
	m_textColors.push_back(Color::WHITE);
	m_textHeights.push_back(0.0f);
	m_textLayouts.push_back(new TextLayout(""));
	m_createdTime = SDL_GetTicks();
	m_bgColor = Color4f(0.2f, 0.2f, 0.6f, 1.0f);
	m_outlineColor = Color4f(0, 0, .8f, 1.0f);
}

ToolTip::~ToolTip()
{
	for (unsigned int i = 0; i < m_textLayouts.size(); ++i) {
		delete m_textLayouts[i];
	}
}

void ToolTip::CalcSize()
{
	float total_size[2] = { 0.0f, 0.0f };
	for (unsigned int i = 0; i < m_textLines.size(); ++i) {
		float size[2];
		m_textLayouts[i]->MeasureSize(400.0, size);
		size[0] += 2 * TOOLTIP_PADDING;
		m_textHeights[i] = size[1];
		total_size[0] = std::max(size[0], total_size[0]);
		total_size[1] += size[1];
	}
	SetSize(total_size[0], total_size[1]);
}

void ToolTip::SetText(const char *text)
{
	m_textLines[0] = text;
	if (m_textLayouts[0]) delete m_textLayouts[0];
	m_textLayouts[0] = new TextLayout(text);
	CalcSize();
}

void ToolTip::SetText(std::string &text)
{
	SetText(text.c_str());
}

void ToolTip::Draw()
{
	PROFILE_SCOPED()
	if (m_owner && !m_owner->IsVisible())
		return;

	float size[2];
	int age = SDL_GetTicks() - m_createdTime;
	float alpha = std::min(age / FADE_TIME_MS, 0.75f);

	Graphics::Renderer *r = Gui::Screen::GetRenderer();
	r->SetRenderState(Gui::Screen::alphaBlendState);

	GetSize(size);
	m_bgColor.a = static_cast<Uint8>(alpha * 255.0f);
	Theme::DrawRect(vector2f(0.f), vector2f(size[0], size[1]), m_bgColor, Screen::alphaBlendState);

	const vector3f outlineVts[] = {
		vector3f(size[0], 0, 0),
		vector3f(size[0], size[1], 0),
		vector3f(0, size[1], 0),
		vector3f(0, 0, 0)
	};
	m_outlineColor.a = static_cast<Uint8>(alpha * 255.0f);
	r->DrawLines(4, &outlineVts[0], m_outlineColor, Screen::alphaBlendState, Graphics::LINE_LOOP);

	Graphics::Renderer::MatrixTicket ticket(r, Graphics::MatrixMode::MODELVIEW);

	r->Translate(TOOLTIP_PADDING, 0.0f, 0.0f);
	for (unsigned int i = 0; i < m_textLines.size(); ++i) {
		r->Translate(0.0f, i > 0? m_textHeights[i - 1] : 0, 0);
		m_textLayouts[i]->Render(size[0] - 2 * TOOLTIP_PADDING, m_textColors[i]);
	}
}

void ToolTip::GetSizeRequested(float size[2])
{
	m_textLayouts[0]->MeasureSize(size[0] - 2*TOOLTIP_PADDING, size);
	size[0] += 2*TOOLTIP_PADDING;
}

void ToolTip::AddTextLine(std::string& text, Color color)
{
	m_textLines.push_back(text);
	m_textLayouts.push_back(new TextLayout(text.c_str()));
	m_textColors.push_back(color);
	m_textHeights.push_back(0.0f);
	CalcSize();
}

}
