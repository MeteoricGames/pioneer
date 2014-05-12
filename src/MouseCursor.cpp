// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "MouseCursor.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "graphics/RenderState.h"
#include "graphics/TextureBuilder.h"
#include "Pi.h"
#include "WorldView.h"

MouseCursor::MouseCursor(Graphics::Renderer* renderer) : m_renderer(renderer),
	m_visible(true), m_type(MCT_NORMAL),
	m_pos(vector2f(0.0f, 0.0f)),
	m_flightCursor(vector2f(0.0f, 0.0f))
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

	// Mouse flight zone
	b = Graphics::TextureBuilder::UI("icons/mouse_flight_zone.png");
	m_mouseFlightZone.reset(new Gui::TexturedQuad(b.GetOrCreateTexture(Gui::Screen::GetRenderer(), "ui")));

	SDL_ShowCursor(0);

	Graphics::RenderStateDesc rsd;
	rsd.blendMode = Graphics::BlendMode::BLEND_ALPHA;
	rsd.depthTest = false;
	rsd.depthWrite = false;
	m_cursorRS = m_renderer->CreateRenderState(rsd);
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
	if (m_type == MCT_NORMAL) {
		int abs_mouse_state[2];
		SDL_GetMouseState(&abs_mouse_state[0], &abs_mouse_state[1]);
		m_pos.x = abs_mouse_state[0];
		m_pos.y = abs_mouse_state[1];
	} else {
		int rel_mouse_state[2];
		SDL_GetRelativeMouseState(&rel_mouse_state[0], &rel_mouse_state[1]);
		m_flightCursor.x += static_cast<float>(rel_mouse_state[0]);
		m_flightCursor.y += static_cast<float>(rel_mouse_state[1]);
	}
}

void MouseCursor::Draw()
{
	if(m_visible) {
		int c = static_cast<int>(m_type);
		Graphics::Renderer::MatrixTicket mt1(m_renderer, Graphics::MatrixMode::MODELVIEW);
		Graphics::Renderer::MatrixTicket mt2(m_renderer, Graphics::MatrixMode::PROJECTION);
		float window_width = m_renderer->GetWindow()->GetWidth();
		float window_height = m_renderer->GetWindow()->GetHeight();
		float ui_width = Gui::Screen::GetWidth();
		float ui_height = Gui::Screen::GetHeight();

		m_renderer->SetOrthographicProjection(0,
			window_width, window_height, 0, -1, 1);
		m_renderer->SetTransform(matrix4x4d::Identity());
		m_renderer->SetRenderState(m_cursorRS);

		// Draw mouse flight control zone
		if (m_type == MCT_FLIGHT) {
			WorldView* wv = dynamic_cast<WorldView*>(Pi::GetView());
			if (wv) {
				float diameter = MouseFlightZoneDiameter * window_width;
				m_mouseFlightZonePos.x = (window_width - diameter) / 2.0f;
				m_mouseFlightZonePos.y = (window_height - diameter) / 2.0f;
				m_mouseFlightZoneSize.x = diameter;
				m_mouseFlightZoneSize.y = diameter;
				m_mouseFlightZone->Draw(Pi::renderer, m_mouseFlightZonePos, m_mouseFlightZoneSize);
			}

		}
		
		// Draw cursor
		if (m_type == MCT_NORMAL) {
			m_vCursor[c]->Draw(m_renderer,
				m_pos + vector2f(-m_vHotspot[c].x * m_vSize[c].x,
				-m_vHotspot[c].y * m_vSize[c].y),
				m_vSize[c]);
		} else { // MCT_FLIGHT
			vector2f mouse_pos = m_flightCursor;
			if (mouse_pos.LengthSqr() > 1.0f) {
				float zone_radius = (MouseFlightZoneDiameter * window_width / 2.0f) - (m_vSize[c].y / 2.0f);
				float mouse_distance;
				vector2f mouse_dir = mouse_pos.Normalized(mouse_distance);
				if (mouse_distance > zone_radius) {
					mouse_pos = mouse_dir * zone_radius;
				}
				m_flightCursor = mouse_pos;
			}
			mouse_pos.x += -m_vHotspot[c].x * m_vSize[c].x + window_width / 2.0f;
			mouse_pos.y += -m_vHotspot[c].y * m_vSize[c].y + window_height / 2.0f;
			m_vCursor[c]->Draw(m_renderer, mouse_pos, 
				m_vSize[c]);
		}
	}
}

void MouseCursor::SetVisible(bool visible)
{
	m_visible = visible;
}

void MouseCursor::SetType(MouseCursorType type)
{
	if (m_type != type) {
		m_flightCursor = vector2f(0.0f, 0.0f);
		m_type = type;
		if (m_type == MCT_FLIGHT) {
			Pi::SetMouseGrab(true);
		} else {
			Pi::SetMouseGrab(false);
		}
	}
}

void MouseCursor::Reset()
{
	SetType(MCT_NORMAL);
}
