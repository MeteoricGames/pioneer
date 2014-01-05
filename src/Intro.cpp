// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Intro.h"
#include "Pi.h"
#include "Lang.h"
#include "Easing.h"
#include "graphics/Renderer.h"
#include "graphics/TextureBuilder.h"
#include "graphics/Graphics.h"
#include "scenegraph/SceneGraph.h"
#include <algorithm>

struct PiRngWrapper {
	unsigned int operator()(unsigned int n) {
		return Pi::rng.Int32(n);
	}
};

Intro::Intro(Graphics::Renderer *r, int width, int height)
: Cutscene(r, width, height)
{
	using Graphics::Light;

	m_background.reset(new Background::Container(r, UNIVERSE_SEED));
	m_ambientColor = Color(0);

	const Color one = Color::WHITE;
	const Color two = Color(77, 77, 204, 0);
	m_lights.push_back(Light(Graphics::Light::LIGHT_DIRECTIONAL, vector3f(0.f, 0.3f, 1.f), one, one));
	m_lights.push_back(Light(Graphics::Light::LIGHT_DIRECTIONAL, vector3f(0.f, -1.f, 0.f), two, Color::BLACK));

	SceneGraph::ModelSkin skin;
	skin.SetDecal("pioneer");
	skin.SetLabel(Lang::PIONEER);

	for (std::vector<ShipType::Id>::const_iterator i = ShipType::player_ships.begin(); i != ShipType::player_ships.end(); ++i) {
		SceneGraph::Model *model = Pi::FindModel(ShipType::types[*i].modelName)->MakeInstance();
		skin.SetRandomColors(Pi::rng);
		skin.Apply(model);
		model->SetThrust(vector3f(0.f, 0.f, -0.6f), vector3f(0.f));
		const Uint32 numMats = model->GetNumMaterials();
		for( Uint32 m=0; m<numMats; m++ ) {
			RefCountedPtr<Graphics::Material> mat = model->GetMaterialByIndex(m);
			mat->specialParameter0 = nullptr;
		}
		m_models.push_back(model);
	}

	PiRngWrapper rng;
	std::random_shuffle(m_models.begin(), m_models.end(), rng);

	m_state = STATE_SELECT;
	m_modelIndex = 0;
}

Intro::~Intro()
{

}

void Intro::Draw(float _time)
{

	m_renderer->SetPerspectiveProjection(75, m_aspectRatio, 1.f, 10000.f);
	m_renderer->SetTransform(matrix4x4f::Identity());

	m_renderer->SetDepthTest(true);
	m_renderer->SetDepthWrite(true);

	glPushAttrib(GL_ALL_ATTRIB_BITS & (~GL_POINT_BIT));

	const Color oldSceneAmbientColor = m_renderer->GetAmbientColor();
	m_renderer->SetAmbientColor(m_ambientColor);
	m_renderer->SetLights(m_lights.size(), &m_lights[0]);

	// XXX all this stuff will be gone when intro uses a Camera
	// rotate background by time, and a bit extra Z so it's not so flat
	matrix4x4d brot = matrix4x4d::RotateXMatrix(-0.25*_time) * matrix4x4d::RotateZMatrix(0.6);
	m_background->Draw(m_renderer, brot);

	glPopAttrib();
}
