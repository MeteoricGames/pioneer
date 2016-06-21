// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "ThrusterTrail.h"
#include "graphics/RendererGL2.h"
#include "graphics/VertexArray.h"
#include "graphics/gl2/MultiMaterial.h"
#include "graphics/gl2/GL2Material.h"
#include "graphics/RenderState.h"
#include "Pi.h"
#include "WorldView.h"
#include "Camera.h"
#include "CameraController.h"
#include "Player.h"

const float UPDATE_INTERVAL = 0.016f;
const Uint16 MAX_POINTS = 50;

ThrusterTrail::ThrusterTrail(Graphics::Renderer* renderer, Body *b, const Color& c, 
							 vector3f position, float scale)
: m_renderer(renderer)
, m_body(b)
, m_color(c)
, m_updateTime(0.f)
, m_position(position)
{
	m_currentFrame = b->GetFrame();
	m_trailGeometry.reset(new Graphics::VertexArray(
		Graphics::VertexAttrib::ATTRIB_POSITION | 
		Graphics::VertexAttrib::ATTRIB_DIFFUSE | 
		Graphics::VertexAttrib::ATTRIB_UV0, 2 + (MAX_POINTS * 2)));
	for(unsigned int i = 0; i < 2 + (MAX_POINTS * 2); ++i) {
		m_trailGeometry->Add(vector3f(0.0f, 0.0f, 0.0f), Color(255, 255, 255, 255), vector2f(0.0f, 0.0f));
	}

	m_scale = std::max<float>(0.0f, scale - THRUSTER_TRAILS_LOWER_BOUND) / 
		(THRUSTER_TRAILS_UPPER_BOUND - THRUSTER_TRAILS_LOWER_BOUND);

	Graphics::RenderStateDesc rsd;
	rsd.blendMode = Graphics::BlendMode::BLEND_ALPHA_ONE;
	rsd.depthWrite = false;
	rsd.depthTest = true;
	m_renderState = m_renderer->CreateRenderState(rsd);
}

void ThrusterTrail::Update(float time)
{
	//record position
	m_updateTime += time;
	if (m_updateTime > UPDATE_INTERVAL) {
		m_updateTime = 0.f;
		const Frame *bodyFrame = m_body->GetFrame();
		if( bodyFrame == m_currentFrame ) {
			m_trailPoints.push_back(m_body->GetInterpPosition());
		}
	}

	while (m_trailPoints.size() > MAX_POINTS) {
		m_trailPoints.pop_front();
	}

	// Update geometry
	m_render = true;
	const vector3d offset = m_body->GetOrient() * vector3d(m_position);
	if (m_trailPoints.size() > 1) {
		const vector3d vpos = m_transform * (m_body->GetInterpPosition() + offset);
		m_transform[12] = vpos.x;
		m_transform[13] = vpos.y;
		m_transform[14] = vpos.z;
		m_transform[15] = 1.0;

		m_trailVertices.clear();
		m_trailColors.clear();
		m_trailUVs.clear();
		const vector3d curpos = m_body->GetInterpPosition();
		m_trailVertices.push_back(vector3f(0.f));
		m_trailColors.push_back(Color(0));
		m_trailUVs.push_back(vector2f(0.f));
		// R: Alpha for the first gradient, whole trail is lit with the hottest color
		// G: Alpha for the core gradient, core disappears gradually
		// B:
		// A: Used for transparency of the whole trail
		float trail_r = 1.0f;
		float trail_g = 1.0f;
		float trail_a = 1.0f;
		const float decr_r = 1.0f / (m_trailPoints.size() * 0.09345f);
		const float decr_g = 1.0f / (m_trailPoints.size() * 0.76791f);
		const float decr_a = 1.0f / m_trailPoints.size();
		//Color trail_color = m_color;
		Color trail_color(255, 255, 255, 255);

		// Is this necessary? its an extra reset
		for (Uint16 i = m_trailPoints.size()-1; i > 0; i--) {
			m_trailVertices.push_back(-vector3f(curpos - m_trailPoints[i]));
			trail_r -= decr_r; 
			if(trail_r < 0.0f) {
				trail_r = 0.0f;
			}
			trail_g -= decr_g; 
			if(trail_g < 0.0f) {
				trail_g = 0.0f;
			}
			trail_a -= decr_a;
			if(trail_a < 0.0f) {
				trail_a = 0.0f;
			}
			m_trailColors.push_back(trail_color);
			m_trailColors.back().r = Uint8(trail_r * 255.0f);
			m_trailColors.back().g = Uint8(trail_g * 255.0f);
			m_trailColors.back().a = Uint8(trail_a * 255.0f);
			m_trailUVs.push_back(vector2f(0.0f, 0.0f));
		}

		const vector3f v_cam = vector3f(Pi::worldView->GetCameraController()->GetCameraContext()->GetOrient().VectorZ());
		const vector3f v_zero = vector3f(0.0f, 0.0f, 0.0f);
		vector3f v_trail = vector3f(-m_body->GetOrient().VectorZ());
		float angle = acosf(v_cam.Dot(v_trail));
		// Exhaust trails don't render if:
		// - Looking perfectly perpendicular towards the thrusters
		// - Going forward at less than 5 m/s
		// - Going backwards
		if(angle < 0.01f || 
			angle > 3.14f ||
			m_body->GetVelocity().Dot(m_body->GetOrient().VectorZ()) >= -5.0f) {
			m_render = false;
			return;
		}
		const vector3f v_first_extend = vector3f(v_cam.Cross(v_trail).Normalized());
		vector3f v_extend = vector3f(0.0f, 0.0f, 0.0f);
		unsigned int vc = 2;

		// First segment doesn't change
		const vector2f uv1(0.0f, 0.0f);
		const vector2f uv2(0.0f, 1.0f);
		float size = ((m_scale * 0.9f) + 0.1f) * THRUSTER_TRAILS_MAX_WIDTH;
		m_trailGeometry->Set(0, (-v_first_extend * size), trail_color, uv1);
		m_trailGeometry->Set(1, (v_first_extend * size), trail_color, uv2);

		vector3f v_prev = v_zero;
	  	for(unsigned int i = 0; i < m_trailVertices.size(); ++i) {
			Color& c = m_trailColors[i];
			if(m_trailVertices[i].LengthSqr() <= 0.01 || (m_trailVertices[i] - v_prev).LengthSqr() < 0.01) {
				continue;
			}
			if(i > 0) {
				v_trail = m_trailVertices[i] - m_trailVertices[i - 1];
			}
			v_extend = v_cam.Cross(v_trail).Normalized();
			m_trailGeometry->Set(vc, m_trailVertices[i] + (v_extend * size), c, uv1);
			m_trailGeometry->Set(vc + 1, m_trailVertices[i] - (v_extend * size), c, uv2);
			vc += 2;
			v_prev = m_trailVertices[i];
		}
		m_trailVertexCount = vc;
	} else {
		m_render = false;
	}
}

void ThrusterTrail::Render(Graphics::Material *material)
{
	if (m_render && m_trailPoints.size() > 1 && m_trailVertexCount > 0) {
		m_renderer->SetTransform(m_transform);
		m_renderer->DrawTriangles(m_trailVertexCount, m_trailGeometry.get(), m_renderState, 
			material, Graphics::TRIANGLE_STRIP);
	}
}

void ThrusterTrail::Reset(Frame *newFrame)
{
	m_currentFrame = newFrame;
	ClearTrail();
}

void ThrusterTrail::ClearTrail()
{
	m_trailPoints.clear();
}
