// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "MouseCursor.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "graphics/TextureBuilder.h"

MouseCursor::MouseCursor() : m_visible(true)
{
	Graphics::TextureBuilder b = Graphics::TextureBuilder::UI("mouse_normal.png");
	m_cursor.reset(new Gui::TexturedQuad(b.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui")));
	const Graphics::TextureDescriptor &d = b.GetDescriptor();	
	m_hotspot = vector2f(20.0f/64.0f, 6.0f/64.0f);
	m_pos = vector2f(0.0f, 0.0f);
	m_size = vector2f(d.dataSize.x * d.texSize.x, d.dataSize.y * d.texSize.y);
	SDL_ShowCursor(0);
}

MouseCursor::~MouseCursor()
{
	SDL_ShowCursor(1);
}

void MouseCursor::Update()
{
	int abs_mouse_state[2];
	SDL_GetMouseState(&abs_mouse_state[0], &abs_mouse_state[1]);
	m_pos.x = abs_mouse_state[0];
	m_pos.y = abs_mouse_state[1];
}

void MouseCursor::Draw(Graphics::Renderer* renderer)
{
	if(m_visible) {
		matrix4x4f modelMatrix = renderer->GetCurrentModelView();
		matrix4x4f projMatrix = renderer->GetCurrentProjection();

		renderer->SetOrthographicProjection(0, 
			renderer->GetWindow()->GetWidth(), 
			renderer->GetWindow()->GetHeight(), 0, -1, 1);
		renderer->SetTransform(matrix4x4d::Identity());
		renderer->SetBlendMode(Graphics::BLEND_ALPHA);
		m_cursor->Draw(renderer, m_pos + vector2f(-m_hotspot.x * m_size.x, -m_hotspot.y * m_size.y), m_size);
		renderer->SetBlendMode(Graphics::BLEND_SOLID);

		renderer->SetProjection(projMatrix);
		renderer->SetTransform(modelMatrix);
	}
}

void MouseCursor::SetVisible(bool visible)
{
	m_visible = visible;
}