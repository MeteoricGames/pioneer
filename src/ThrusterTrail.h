// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _THRUSTERTRAIL_H
#define _THRUSTERTRAIL_H

#include "libs.h"
#include "Body.h"

namespace Graphics {
	class Renderer;
	class RenderState;
	class Material;
	class VertexArray;
	namespace GL2 {
		class Material;
	};
};

static const float THRUSTER_TRAILS_LOWER_BOUND = 1.0f; // thrusters at this scale will be given 0.1 trail size, for less than lower bound scale no thruster will be created.
static const float THRUSTER_TRAILS_UPPER_BOUND = 2.0f; // thrusters at this scale or more will be given 1.0+ trail size
static const Color THRUSTER_TRAILS_COLOR = Color(32, 32, 128, 255);
static const float THRUSTER_TRAILS_MAX_WIDTH = 0.4f;

class ThrusterTrail
{
public:
	ThrusterTrail(Graphics::Renderer* renderer, Body *b, const Color&, vector3f position, float scale);
	void Update(float time);
	void Render(Graphics::Material *material);
	void Reset(Frame *newFrame);
	void ClearTrail();

	void SetColor(const Color &c) { m_color = c; }
	void SetTransform(const matrix4x4d &t) { m_transform = t; }

private:
	Graphics::Renderer* m_renderer;
	Graphics::RenderState* m_renderState;
	Body *m_body;
	Frame *m_currentFrame;
	Color m_color;
	matrix4x4d m_transform;
	std::deque<vector3d> m_trailPoints;
	std::unique_ptr<Graphics::VertexArray> m_trailGeometry;
	std::vector<vector3f> m_trailVertices;
	std::vector<Color> m_trailColors;
	std::vector<vector2f> m_trailUVs;
	vector3f m_position;
	float m_updateTime;
	float m_scale;
	unsigned int m_trailVertexCount;
	bool m_render;
};

#endif
