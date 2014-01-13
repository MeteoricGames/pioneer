// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "MouseCursor.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "graphics/TextureBuilder.h"

MouseCursor::MouseCursor() : m_visible(true), m_type(MCT_NORMAL), m_pos(vector2f(0.0f, 0.0f))
{
	Gui::TexturedQuad *normal_cursor, *flight_cursor;

	Graphics::TextureBuilder b = Graphics::TextureBuilder::UI("icons/mouse_normal.png");
	normal_cursor = new Gui::TexturedQuad(b.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui"));
	m_vCursor.push_back(normal_cursor);
	const Graphics::TextureDescriptor &d = b.GetDescriptor();
	m_vHotspot.push_back(vector2f(3.0f/33.0f, 3.0f/33.0f));
	m_vSize.push_back(vector2f(d.dataSize.x * d.texSize.x, d.dataSize.y * d.texSize.y));

	b = Graphics::TextureBuilder::UI("icons/mouse_follow.png");
	flight_cursor = new Gui::TexturedQuad(b.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui"));
	m_vCursor.push_back(flight_cursor);
	const Graphics::TextureDescriptor &d2 = b.GetDescriptor();
	m_vHotspot.push_back(vector2f(16.0f/33.0f, 16.0f/33.0f));
	m_vSize.push_back(vector2f(d2.dataSize.x * d2.texSize.x, d2.dataSize.y * d2.texSize.y));

	SDL_ShowCursor(0);
}

MouseCursor::~MouseCursor()
{
	for(unsigned int i = 0; i < m_vCursor.size(); ++i) {
		delete m_vCursor[i];
	}
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
		int c = static_cast<int>(m_type);
		matrix4x4f modelMatrix = renderer->GetCurrentModelView();
		matrix4x4f projMatrix = renderer->GetCurrentProjection();

		renderer->SetOrthographicProjection(0, 
			renderer->GetWindow()->GetWidth(), 
			renderer->GetWindow()->GetHeight(), 0, -1, 1);
		renderer->SetTransform(matrix4x4d::Identity());
		renderer->SetBlendMode(Graphics::BLEND_ALPHA);
		m_vCursor[c]->Draw(renderer, m_pos + vector2f(-m_vHotspot[c].x * m_vSize[c].x, -m_vHotspot[c].y * m_vSize[c].y), m_vSize[c]);
		renderer->SetBlendMode(Graphics::BLEND_SOLID);

		renderer->SetProjection(projMatrix);
		renderer->SetTransform(modelMatrix);
	}
}

void MouseCursor::SetVisible(bool visible)
{
	m_visible = visible;
}

void MouseCursor::SetType(MouseCursorType type)
{
	m_type = type;
}

void MouseCursor::Reset()
{
	SetType(MCT_NORMAL);
}
