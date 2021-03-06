// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "TerrainBody.h"
#include "GeoSphere.h"
#include "Pi.h"
#include "WorldView.h"
#include "Frame.h"
#include "Game.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"

double oldLen = 0;

TerrainBody::TerrainBody(SystemBody *sbody) :
	Body(),
	m_sbody(0),
	m_mass(0),
	m_geosphere(0)
{
	InitTerrainBody(sbody);
}

TerrainBody::TerrainBody() :
	Body(),
	m_sbody(0),
	m_mass(0),
	m_geosphere(0)
{
}

TerrainBody::~TerrainBody()
{
	if (m_geosphere)
		delete m_geosphere;
}

void TerrainBody::InitTerrainBody(SystemBody *sbody)
{
	assert(!m_sbody);
	m_sbody = sbody;
	m_mass = m_sbody->GetMass();
	if (!m_geosphere)
		m_geosphere = new GeoSphere(sbody);
	m_maxFeatureHeight = (m_geosphere->GetMaxFeatureHeight() + 1.0) * m_sbody->GetRadius();
}

void TerrainBody::Save(Serializer::Writer &wr, Space *space)
{
	Body::Save(wr, space);
	wr.Int32(space->GetIndexForSystemBody(m_sbody));
}

void TerrainBody::Load(Serializer::Reader &rd, Space *space)
{
	Body::Load(rd, space);
	SystemBody *sbody = space->GetSystemBodyByIndex(rd.Int32());
	InitTerrainBody(sbody);
}

void TerrainBody::Render(Graphics::Renderer *renderer, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform)
{
	matrix4x4d ftran = viewTransform;
	vector3d fpos = viewCoords;
	double rad = m_sbody->GetRadius();

	float znear, zfar;
	renderer->GetNearFarRange(znear, zfar);

	double len = fpos.Length();
    if (len != len)
        len = oldLen;
    else
        oldLen = len;
	//objects very far away are downscaled, because they cannot be
	//accurately drawn using actual distances
	int shrink = 0;
	double scale = 1.0f;

	double dist_to_horizon;
	for (;;) {
		if (len < rad) break;		// player inside radius case
		dist_to_horizon = sqrt(len*len - rad*rad);

		if (dist_to_horizon < zfar*0.5) break;

		rad *= 0.25;
		fpos = 0.25*fpos;
		len *= 0.25;
		scale *= 4.0f;
		++shrink;
	}

	vector3d campos = fpos;
	ftran.ClearToRotOnly();
	campos = ftran.InverseOf() * campos;

	campos = campos * (1.0/rad);		// position of camera relative to planet "model"

	std::vector<Camera::Shadow> shadows;
	if( camera ) {
		camera->PrincipalShadows(this, 3, shadows);
		for (std::vector<Camera::Shadow>::iterator it = shadows.begin(), itEnd=shadows.end(); it!=itEnd; ++it) {
			it->centre = ftran * it->centre;
		}
	}

	ftran.Scale(rad, rad, rad);

	// translation not applied until patch render to fix jitter
	m_geosphere->Render(renderer, ftran, -campos, m_sbody->GetRadius(), scale, shadows);

	ftran.Translate(campos.x, campos.y, campos.z);
	SubRender(renderer, ftran, campos);

	//clear depth buffer, shrunk objects should not interact
	//with foreground
	if (shrink)
		renderer->ClearDepthBuffer();
}

void TerrainBody::SetFrame(Frame *f)
{
	if (GetFrame()) {
		GetFrame()->SetPlanetGeom(0, 0);
	}
	Body::SetFrame(f);
	if (f) {
		GetFrame()->SetPlanetGeom(0, 0);
	}
}

double TerrainBody::GetTerrainHeight(const vector3d &pos_) const
{
	double radius = m_sbody->GetRadius();
	if (m_geosphere) {
		return radius * (1.0 + m_geosphere->GetHeight(pos_));
	} else {
		assert(0);
		return radius;
	}
}

bool TerrainBody::IsSuperType(SystemBody::BodySuperType t) const
{
	if (!m_sbody) return false;
	else return m_sbody->GetSuperType() == t;
}
