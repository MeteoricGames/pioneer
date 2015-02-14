// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "GameLog.h"
#include "Colors.h"
#include "StringF.h"
#include "graphics/Renderer.h"

const Uint32 MESSAGE_TIMEOUT  = 8000;
const Uint32 FADE_TIME  = 1000;
const Uint32 FADE_AFTER = MESSAGE_TIMEOUT - FADE_TIME;
const Uint8 MAX_MESSAGES = 9;

GameLog::Message::Message(const std::string &m, Uint32 t)
: msg(m), time(t)
{
}

GameLog::GameLog(RefCountedPtr<Text::TextureFont> font, vector2f scrSize)
: m_font(font)
, m_screenSize(scrSize)
{
	m_lineHeight = m_font->GetHeight();

	// position just above cpanel
	m_offset.x = (scrSize.x / 800.f) * 12.0f; // 170.f;
	m_offset.y = (scrSize.y / 600.f) * (600.0f - 15.0f); // 535.f;
}

void GameLog::Add(const std::string &msg)
{
	m_messages.push_back(Message(msg, 0));

	while (m_messages.size() > MAX_MESSAGES) {
		m_messages.pop_front();
	}
}

void GameLog::Add(const std::string &from, const std::string &msg)
{
	if(from.length() < 3) {
		Add(stringf("%0", msg));
	} else {
		Add(stringf("%0: %1", from, msg));
	}
}

void GameLog::Update(bool paused)
{
	static Uint32 prevTime = 0;
	Uint32 now = SDL_GetTicks();
	Uint32 elapsed = now - prevTime;
	prevTime = now;

	//age messages
	if (!paused) {
		for (auto it = m_messages.begin(); it != m_messages.end();) {
			it->time += elapsed;

			if (it->time >= MESSAGE_TIMEOUT)
				m_messages.erase(it++);
			else
				++it;
		}
	}
}

void GameLog::DrawHudMessages(Graphics::Renderer *r)
{
	Graphics::Renderer::MatrixTicket ticket1(r, Graphics::MatrixMode::MODELVIEW);
	Graphics::Renderer::MatrixTicket ticket2(r, Graphics::MatrixMode::PROJECTION);
	Graphics::RenderStateDesc rsd;
	rsd.depthTest  = false;
	rsd.depthWrite = false;
	Graphics::RenderState *prsd = r->CreateRenderState(rsd);

	//I'd rather render this as one string, but then we can't
	//have per-line fade - markup doesn't support alpha
	r->SetOrthographicProjection(0, m_screenSize.x, m_screenSize.y, 0, -1, 1);
	r->SetTransform(matrix4x4f::Identity());
	r->SetRenderState(prsd);

	const Color &c = Color::PARAGON_GREEN; //Colors::HUD_MESSAGE;

	float y = 0;
	for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it) {
		float alpha = 1.f;
		if (it->time > FADE_AFTER) {
			alpha = 1.0f - (float(it->time - FADE_AFTER) / FADE_TIME);
		}
		m_font->RenderString(it->msg.c_str(), m_offset.x, m_offset.y + y, Color(c.r, c.g, c.b, alpha*255));
		y -= m_lineHeight;
	}
}
