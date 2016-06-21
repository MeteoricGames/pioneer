// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _LIGHT_H
#define _LIGHT_H

#include "Color.h"
#include "vector3.h"

namespace Graphics {

class Light
{
public:
	enum LightType {
		LIGHT_POINT,
		LIGHT_SPOT,
		LIGHT_DIRECTIONAL
	};
	Light();
	Light(LightType t, const vector3f &position, const Color &diffuse, const Color &specular, float r);
	virtual ~Light() {}
	void SetType(LightType t) { m_type = t; }
	void SetPosition(const vector3f &p) { m_position = p; }
	void SetDiffuse(const Color &c) { m_diffuse = c; }
	void SetSpecular(const Color &c) { m_specular = c; }
	void SetRadius(float r) { m_radius = r; }

	LightType GetType() const { return m_type; }
	const vector3f &GetPosition() const { return m_position; }
	const Color &GetDiffuse() const { return m_diffuse; }
	const Color &GetSpecular() const { return m_specular; }
	float GetRadius() const { return m_radius; }

private:
	LightType m_type;
	vector3f m_position;
	Color m_diffuse;
	Color m_specular;
	float m_radius;
};

}

#endif
