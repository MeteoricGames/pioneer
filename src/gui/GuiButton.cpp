// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "libs.h"
#include "Gui.h"

static const float BUTTON_SIZE = 16.f;

namespace Gui {
Button::Button()
{
	m_isPressed = false;
	m_eventMask = EVENT_MOUSEDOWN | EVENT_MOUSEUP | EVENT_MOUSEMOTION;
	SetSize(BUTTON_SIZE, BUTTON_SIZE);
}

Button::~Button()
{
	_m_release.disconnect();
	_m_kbrelease.disconnect();
}

bool Button::OnMouseDown(MouseButtonEvent *e)
{
	if (e->button == SDL_BUTTON_LEFT) {
		m_isPressed = true;
		onPress.emit();
		// wait for mouse release, regardless of where on screen
		_m_release = RawEvents::onMouseUp.connect(sigc::mem_fun(this, &Button::OnRawMouseUp));
	}
	return false;
}

bool Button::OnMouseUp(MouseButtonEvent *e)
{
	if ((e->button == SDL_BUTTON_LEFT) && m_isPressed) {
		m_isPressed = false;
		_m_release.disconnect();
		onRelease.emit();
		onClick.emit();
	}
	return false;
}

void Button::OnActivate()
{
	// activated by keyboard shortcut
	m_isPressed = true;
	_m_kbrelease = RawEvents::onKeyUp.connect(sigc::mem_fun(this, &Button::OnRawKeyUp));
	onPress.emit();
}

void Button::OnRawKeyUp(SDL_KeyboardEvent *e)
{
	if (e->keysym.sym == m_shortcut.sym) {
		m_isPressed = false;
		_m_kbrelease.disconnect();
		onRelease.emit();
		onClick.emit();
	}
}

void Button::OnRawMouseUp(MouseButtonEvent *e)
{
	if (e->button == SDL_BUTTON_LEFT) {
		m_isPressed = false;
		_m_release.disconnect();
		onRelease.emit();
	}
}

void SolidButton::GetSizeRequested(float size[2])
{
	size[0] = size[1] = BUTTON_SIZE;
}

void TransparentButton::GetSizeRequested(float size[2])
{
	size[0] = size[1] = BUTTON_SIZE;
}

void SolidButton::Draw()
{
	PROFILE_SCOPED()
	float size[2];
	GetSize(size);
	if (IsPressed()) {
		Theme::DrawIndent(size, Screen::alphaBlendState);
	} else {
		Theme::DrawOutdent(size, Screen::alphaBlendState);
	}
}
void TransparentButton::Draw()
{
	PROFILE_SCOPED()
	float size[2];
	GetSize(size);
	Theme::DrawHollowRect(size, Color::WHITE, Screen::alphaBlendState);
}

LabelButton::LabelButton(Label *label): Button()
{
	m_label = label;
	m_padding = 2.0f;
	m_padding_y = 0.0f;
	onSetSize.connect(sigc::mem_fun(this, &LabelButton::OnSetSize));
}

LabelButton::~LabelButton() { delete m_label; }

void LabelButton::GetSizeRequested(float size[2])
{
	m_label->GetSizeRequested(size);
	size[0] += 2.0f * m_padding;
	size[1] += 2.0f * m_padding_y;
}

void LabelButton::Draw()
{
	PROFILE_SCOPED()
	float size[2];
	GetSize(size);

	if (IsPressed()) {
		Theme::DrawIndent(size, Screen::alphaBlendState);
	} else {
		Theme::DrawOutdent(size, Screen::alphaBlendState);
	}

	Graphics::Renderer *r = Gui::Screen::GetRenderer();
	Graphics::Renderer::MatrixTicket ticket(r, Graphics::MatrixMode::MODELVIEW);

	r->Translate(m_padding, m_padding_y, 0);
	m_label->Draw();
}

void LabelButton::OnSetSize()
{
	float size[2];
	GetSize(size);

	m_label->SetSize(size[0]-2*m_padding, size[1]);
}

ClickableLabel::ClickableLabel(Label *label, const char *imageFileName, Color color) : LabelButton(label)
{
	m_img = new Image(imageFileName); // TODO: optimize it shouldnt be needed to load the texture everytime we create a button.
	m_img->SetModulateColor(color);
	SizeImage();
}

ClickableLabel::~ClickableLabel()
{
	delete m_img;
}

void ClickableLabel::Draw()
{
	if (m_img)
		m_img->Draw();
	m_label->Draw();
}

void ClickableLabel::OnSetSize()
{
	float size[2];
	GetSize(size);

	//m_label->SetSize(size[0] - 2 * m_padding, size[1]); // Doesn't seem to work.
}

void ClickableLabel::SizeImage()
{
	float size[2];
	GetSize(size);

	m_img->SetSize(size[1] / 2, size[1] / 2);
	m_img->SetPosition(vector2f(0.0f, 1.0f));
}

void ClickableLabel::SetLabel(Label* label) {
    m_label = label;
}

}
