// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "HyperspaceCloud.h"
#include "libs.h"
#include "Game.h"
#include "Lang.h"
#include "perlin.h"
#include "Pi.h"
#include "Player.h"
#include "Serializer.h"
#include "Ship.h"
#include "Space.h"
#include "graphics/Graphics.h"
#include "graphics/Material.h"
#include "graphics/Renderer.h"
#include "graphics/VertexArray.h"
#include "graphics/RenderState.h"
#include "MainMaterial.h"
#include "WorldView.h"

using namespace Graphics;

HyperspaceCloud::HyperspaceCloud(Ship *s, double dueDate, HyperspaceCloudType cloud_type)
{
	m_flags = Body::FLAG_CAN_MOVE_FRAME |
		  Body::FLAG_LABEL_HIDDEN;
	m_ship = s;
    m_sbody = nullptr;
	SetPhysRadius(0.0);
	SetClipRadius(1200.0);
	m_vel = (s ? s->GetVelocity() : vector3d(0.0));
	m_birthdate = Pi::game->GetTime();
	m_due = dueDate;
    SetCloudType(cloud_type);
	InitGraphics();
}

HyperspaceCloud::HyperspaceCloud()
{
    m_flags = Body::FLAG_CAN_MOVE_FRAME |
        Body::FLAG_LABEL_HIDDEN;
	m_ship = nullptr;
    m_sbody = nullptr;
	SetPhysRadius(0.0);
	SetClipRadius(1200.0);
    m_vel = vector3d(0.0);
    m_birthdate = Pi::game? Pi::game->GetTime() : 0.0;
    m_due = 0.0;
    SetCloudType(EHCT_ARRIVAL);
	InitGraphics();
}

HyperspaceCloud::HyperspaceCloud(SystemBody* system_body, HyperspaceCloudType cloud_type = EHCT_PERMANENT)
{
    // This should only be called for permanent clouds
    assert(cloud_type == EHCT_PERMANENT);
    m_ship = nullptr;
    SetPhysRadius(0.0);
    SetClipRadius(1200.0);
    m_vel = vector3d(0.0);
    m_birthdate = 0.0;
    m_due = 0.0;
    SetCloudType(EHCT_PERMANENT);
    InitGraphics();
    m_sbody = system_body;
}

void HyperspaceCloud::InitGraphics()
{
	m_graphic.vertices.reset(new Graphics::VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE));

	Graphics::MaterialDescriptor desc;
	desc.vertexColors = true;
	if(Graphics::Hardware::GL3()) {
		m_graphic.material.reset(new MainMaterial(Pi::renderer, desc));
	} else {
		m_graphic.material.reset(Pi::renderer->CreateMaterial(desc));
	}

	Graphics::RenderStateDesc rsd;
	rsd.blendMode  = BLEND_ALPHA_ONE;
	rsd.depthWrite = false;
	m_graphic.renderState = Pi::renderer->CreateRenderState(rsd);
}

HyperspaceCloud::~HyperspaceCloud()
{
	if (m_ship) delete m_ship;
}

void HyperspaceCloud::SetCloudType(HyperspaceCloudType cloud_type)
{
    m_cloudType = cloud_type;
    switch(m_cloudType) {
        case EHCT_ARRIVAL:
	        SetLabel(Lang::HYPERSPACE_ARRIVAL_CLOUD);
            break;

        case EHCT_DEPARTURE:
            SetLabel(Lang::HYPERSPACE_DEPARTURE_CLOUD);
            break;
        
        case EHCT_PERMANENT:
            SetLabel(Lang::HYPERSPACE_PERMA_CLOUD);
            break;
    }
}

void HyperspaceCloud::Save(Serializer::Writer &wr, Space *space)
{
	Body::Save(wr, space);
	wr.Vector3d(m_vel);
	wr.Double(m_birthdate);
	wr.Double(m_due);
	wr.Byte(static_cast<Uint8>(m_cloudType));
	wr.Bool(m_sbody != nullptr);
	if (m_sbody) {
		wr.Int32(space->GetIndexForSystemBody(m_sbody));
	}
	wr.Bool(m_ship != 0);
	if (m_ship) {
		m_ship->Serialize(wr, space);
	}
}

void HyperspaceCloud::Load(Serializer::Reader &rd, Space *space)
{
	Body::Load(rd, space);
	m_vel = rd.Vector3d();
	m_birthdate = rd.Double();
	m_due = rd.Double();
    SetCloudType(static_cast<HyperspaceCloudType>(rd.Byte()));
	//------------------- SAVE #79 PATCH
	if (Game::s_loadedGameVersion >= 79) {
		if(rd.Bool()) {
			m_sbody = space->GetSystemBodyByIndex(rd.Int32());
		}
	} else {
		m_sbody = nullptr;
	}
	//------------------- PATCH 79 END
	if (rd.Bool()) {
		m_ship = static_cast<Ship*>(Body::Unserialize(rd, space));
	}
}

void HyperspaceCloud::PostLoadFixup(Space *space)
{
	Body::PostLoadFixup(space);
	if (m_ship) {
		m_ship->PostLoadFixup(space);
	}
}

void HyperspaceCloud::TimeStepUpdate(const float timeStep)
{
	SetPosition(GetPosition() + m_vel * timeStep);

	if (SupportsArrival() && m_ship && (m_due < Pi::game->GetTime())) 
    {
		// spawn ship
		// XXX some overlap with Space::DoHyperspaceTo(). should probably all
		// be moved into EvictShip()
		m_ship->SetVisible(true);
		m_ship->SetPosition(GetPosition());
		m_ship->SetVelocity(m_vel);
		m_ship->SetOrient(matrix3x3d::Identity());
		m_ship->SetFrame(GetFrame());
		Pi::game->GetSpace()->AddBody(m_ship);

		if (Pi::player->GetNavTarget() == this && !Pi::player->GetCombatTarget()) {
			Pi::player->SetCombatTarget(m_ship, Pi::player->GetSetSpeedTarget() == this);
        }

		m_ship->EnterSystem();

		m_ship = nullptr;
		m_due = 0.0;
	}
}

bool HyperspaceCloud::ReceiveShip(Ship* ship, double due)
{
	assert(IsPermanent());
	if(HasShip() || !ship) {
		return false;
	}
	m_ship = ship;
	m_ship->SetVisible(false);
	m_due = due;
	m_ship->SetFrame(GetFrame());
	// TO TEST: testing spawn ships in hyperspace
	return true;
}

Ship *HyperspaceCloud::EvictShip()
{
	Ship *s = m_ship;
	m_ship = 0;
	return s;
}

static void make_circle_thing(VertexArray &va, float radius, const Color &colCenter, const Color &colEdge)
{
	va.Add(vector3f(0.f, 0.f, 0.f), colCenter);
	for (float ang=0; ang<float(M_PI)*2.f; ang+=0.1f) {
		va.Add(vector3f(radius*sin(ang), radius*cos(ang), 0.0f), colEdge);
	}
	va.Add(vector3f(0.f, radius, 0.f), colEdge);
}

const char* HyperspaceCloud::GetCloudTypeString() const
{
    switch(m_cloudType) {
        case EHCT_ARRIVAL:
            return Lang::HYPERSPACE_ARRIVAL_CLOUD;
        case EHCT_DEPARTURE:
            return Lang::HYPERSPACE_DEPARTURE_CLOUD;
    }
    return Lang::HYPERSPACE_PERMA_CLOUD;
}

const char* HyperspaceCloud::GetCloudDirString() const
{
    switch (m_cloudType) {
        case EHCT_ARRIVAL:
            return Lang::SOURCE;
        case EHCT_DEPARTURE:
            return Lang::DESTINATION;
    }
    return Lang::BIDIRECTIONAL;
}

void HyperspaceCloud::UpdateInterpTransform(double alpha)
{
	m_interpOrient = matrix3x3d::Identity();
	const vector3d oldPos = GetPosition() - m_vel*Pi::game->GetTimeStep();
	m_interpPos = alpha*GetPosition() + (1.0-alpha)*oldPos;
}

void HyperspaceCloud::Render(Renderer *renderer, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform)
{
	matrix4x4d trans = matrix4x4d::Identity();
	trans.Translate(float(viewCoords.x), float(viewCoords.y), float(viewCoords.z));

	// face the camera dammit
	vector3d zaxis = viewCoords.NormalizedSafe();
	vector3d xaxis = vector3d(0,1,0).Cross(zaxis).Normalized();
	vector3d yaxis = zaxis.Cross(xaxis);
	matrix4x4d rot = matrix4x4d::MakeRotMatrix(xaxis, yaxis, zaxis).InverseOf();
	renderer->SetTransform(trans * rot);

	// precise to the rendered frame (better than PHYSICS_HZ granularity)
	const double preciseTime = Pi::game->GetTime() + Pi::GetGameTickAlpha()*Pi::game->GetTimeStep();

	// Flickering gradient circle, departure clouds are red and arrival clouds blue
    // permanent clouds are green.
	// XXX could just alter the scale instead of recreating the model
	float radius = 250.0f + 25.0f*float(noise(10.0*preciseTime, 0, 0));
	m_graphic.vertices->Clear();
    Color outerColor;
    switch(m_cloudType) {
        case EHCT_ARRIVAL:
            outerColor = Color::BLUE;
            break;

        case EHCT_DEPARTURE:
            outerColor = Color::YELLOW;
            break;

        case EHCT_PERMANENT:
			if(Pi::worldView) {
				outerColor = Pi::worldView->GetHyperCloudTweak().color;
				radius *= Pi::worldView->GetHyperCloudTweak().size;
			} else {
				outerColor = Color::GREEN;
			}
			break;
    }
	outerColor.a = 0;
	make_circle_thing(*m_graphic.vertices.get(), radius, Color::WHITE, outerColor);
	renderer->DrawTriangles(m_graphic.vertices.get(), m_graphic.renderState, m_graphic.material.get(), TRIANGLE_FAN);
}
