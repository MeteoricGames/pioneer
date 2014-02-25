// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Background.h"
#include "Frame.h"
#include "FileSystem.h"
#include "Game.h"
#include "perlin.h"
#include "Pi.h"
#include "Player.h"
#include "Space.h"
#include "galaxy/StarSystem.h"
#include "graphics/Graphics.h"
#include "graphics/Material.h"
#include "graphics/Renderer.h"
#include "graphics/StaticMesh.h"
#include "graphics/Surface.h"
#include "graphics/VertexArray.h"
#include "graphics/TextureBuilder.h"
#include "StringF.h"

#include <SDL_stdinc.h>
#include <sstream>
#include <iostream>

using namespace Graphics;

namespace
{
	static std::unique_ptr<Graphics::Texture> s_defaultCubeMap;

	static Uint32 GetNumSkyboxes()
	{
		char filename[1024];
		snprintf(filename, sizeof(filename), "textures/cube");
		std::vector<FileSystem::FileInfo> fileList;
		FileSystem::gameDataFiles.ReadDirectory(filename, fileList);

		const char *itemMask = "ub";

		Uint32 num_matching = 0;
		for (std::vector<FileSystem::FileInfo>::const_iterator it = fileList.begin(), itEnd = fileList.end(); it!=itEnd; ++it) {
			if (starts_with((*it).GetName(), itemMask)) {
				++num_matching;
			}
		}
		return num_matching;
	}
};

namespace Background
{

void BackgroundElement::SetIntensity(float intensity)
{
	m_material->emissive = Color(intensity*255);
}

UniverseBox::UniverseBox(Graphics::Renderer *r)
{
	m_renderer = r;
	Init();
}

UniverseBox::~UniverseBox()
{
	delete m_cubemap;
	delete m_model;
}

void UniverseBox::Init()
{
	// Load cubemap
	TextureBuilder texture_builder = TextureBuilder::Cube("textures/cube/ub0.dds");
	m_cubemap = texture_builder.CreateTexture(m_renderer);

	// Create skybox geometry
	VertexArray *box = new VertexArray(ATTRIB_POSITION | ATTRIB_UV0, 36);
	const float vp = 1000.0f;
	// Top +Y
	box->Add(vector3f(-vp,  vp,  vp), vector2f(0.0f, 0.0f));
	box->Add(vector3f(-vp,  vp, -vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp,  vp,  vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f( vp,  vp,  vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f(-vp,  vp, -vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp,  vp, -vp), vector2f(1.0f, 1.0f));
	// Bottom -Y
	box->Add(vector3f(-vp, -vp, -vp), vector2f(0.0f, 0.0f));
	box->Add(vector3f(-vp, -vp,  vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp, -vp, -vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f( vp, -vp, -vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f(-vp, -vp,  vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp, -vp,  vp), vector2f(1.0f, 1.0f));
	// Front -Z
	box->Add(vector3f(-vp,  vp, -vp), vector2f(0.0f, 0.0f));
	box->Add(vector3f(-vp, -vp, -vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp,  vp, -vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f( vp,  vp, -vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f(-vp, -vp, -vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp, -vp, -vp), vector2f(1.0f, 1.0f));
	// Back +Z
	box->Add(vector3f( vp,  vp,  vp), vector2f(0.0f, 0.0f));
	box->Add(vector3f( vp, -vp,  vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f(-vp,  vp,  vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f(-vp,  vp,  vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f( vp, -vp,  vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f(-vp, -vp,  vp), vector2f(1.0f, 1.0f));
	// Right +X
	box->Add(vector3f( vp,  vp, -vp), vector2f(0.0f, 0.0f));
	box->Add(vector3f( vp, -vp, -vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp,  vp,  vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f( vp,  vp,  vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f( vp, -vp, -vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f( vp, -vp,  vp), vector2f(1.0f, 1.0f));
	// Left -X
	box->Add(vector3f(-vp,  vp,  vp), vector2f(0.0f, 0.0f));
	box->Add(vector3f(-vp, -vp,  vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f(-vp,  vp, -vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f(-vp,  vp, -vp), vector2f(1.0f, 0.0f));
	box->Add(vector3f(-vp, -vp,  vp), vector2f(0.0f, 1.0f));
	box->Add(vector3f(-vp, -vp, -vp), vector2f(1.0f, 1.0f));
	m_model = new StaticMesh(TRIANGLES);
	Graphics::MaterialDescriptor desc;
	desc.effect = EFFECT_SKYBOX;
	m_material.Reset(m_renderer->CreateMaterial(desc));
	m_material->texture0 = m_cubemap;
	m_model->AddSurface(RefCountedPtr<Surface>(new Surface(TRIANGLES, box, m_material)));
	SetIntensity(1.0f);
	
	Graphics::RenderStateDesc rsd;
	rsd.blendMode = Graphics::BlendMode::BLEND_SOLID;
	rsd.depthTest = false;
	rsd.depthWrite = false;
	m_cubeRS = m_renderer->CreateRenderState(rsd);
}

void UniverseBox::Draw()
{
	if(m_cubemap) {
		m_renderer->DrawStaticMesh(m_model, m_cubeRS);
	}
}

void UniverseBox::LoadCubeMap(Random* randomizer)
{
	// Clean old texture
	delete m_cubemap;
	m_cubemap = nullptr;
	
	if(randomizer) {
		int new_ubox_index = randomizer->Int32(0, 4);
		if(new_ubox_index > 0) {
			// Load new one
			std::ostringstream os;
			os << "textures/cube/ub" << (new_ubox_index - 1) << ".dds";
			TextureBuilder texture_builder = TextureBuilder::Cube(os.str().c_str());
			m_cubemap = texture_builder.CreateTexture(m_renderer);
			m_material->texture0 = m_cubemap;
		}
	}
}

void UniverseBox::SetIntensity(float intensity)
{
	fIntensity = intensity;
	m_material->specialParameter0 = &fIntensity;
}



Starfield::Starfield(Graphics::Renderer *renderer, Random &rand)
{
	m_renderer = renderer;
	Init();
	Fill(rand);
}

void Starfield::Init()
{
	// reserve some space for positions, colours
	VertexArray *stars = new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE, BG_STAR_MAX);
	m_model.reset(new StaticMesh(POINTS));
	Graphics::MaterialDescriptor desc;
	desc.effect = Graphics::EFFECT_STARFIELD;
	desc.vertexColors = true;
	m_material.Reset(m_renderer->CreateMaterial(desc));
	m_material->emissive = Color::WHITE;
	m_model->AddSurface(RefCountedPtr<Surface>(new Surface(POINTS, stars, m_material)));
		
	Graphics::RenderStateDesc rsd;
	rsd.blendMode = Graphics::BlendMode::BLEND_SOLID;
	rsd.depthTest = false;
	m_starfieldRS = m_renderer->CreateRenderState(rsd);
}

void Starfield::Fill(Random &rand)
{
	VertexArray *va = m_model->GetSurface(0)->GetVertices();
	va->Clear(); // clear if previously filled

	//fill the array
	for (int i=0; i<BG_STAR_MAX; i++) {
		Uint8 col = rand.Double(0.2,0.7)*255;

		// this is proper random distribution on a sphere's surface
		const float theta = float(rand.Double(0.0, 2.0*M_PI));
		const float u = float(rand.Double(-1.0, 1.0));

		va->Add(vector3f(
				1000.0f * sqrt(1.0f - u*u) * cos(theta),
				1000.0f * u,
				1000.0f * sqrt(1.0f - u*u) * sin(theta)
			), Color(col, col, col,	255)
		);
	}
}

void Starfield::Draw()
{
	// XXX would be nice to get rid of the Pi:: stuff here
	if (!Pi::game || Pi::player->GetFlightState() != Ship::HYPERSPACE) {
		m_renderer->DrawStaticMesh(m_model.get(), m_starfieldRS);
	} else {
		// roughly, the multiplier gets smaller as the duration gets larger.
		// the time-looking bits in this are completely arbitrary - I figured
		// it out by tweaking the numbers until it looked sort of right
		double mult = 0.0015 / (Pi::player->GetHyperspaceDuration() / (60.0*60.0*24.0*7.0));

		double hyperspaceProgress = Pi::game->GetHyperspaceProgress();

		VertexArray *va = m_model->GetSurface(0)->GetVertices();
		const vector3d pz = Pi::player->GetOrient().VectorZ();	//back vector
		for (int i=0; i<BG_STAR_MAX; i++) {
			vector3f v(va->position[i]);
			v += vector3f(pz*hyperspaceProgress*mult);

			m_hyperVtx[i*2] = va->position[i] + v;
			m_hyperCol[i*2] = va->diffuse[i];

			m_hyperVtx[i*2+1] = v;
			m_hyperCol[i*2+1] = va->diffuse[i];
		}
		m_renderer->DrawLines(BG_STAR_MAX*2, m_hyperVtx, m_hyperCol, m_starfieldRS);
	}
}

MilkyWay::MilkyWay(Graphics::Renderer *renderer)
{
	m_renderer = renderer;

	m_model.reset(new StaticMesh(TRIANGLE_STRIP));

	//build milky way model in two strips (about 256 verts)
	//The model is built as a generic vertex array first. The renderer
	//will reprocess this into buffered format as it sees fit. The old data is
	//kept around as long as StaticMesh is alive (needed if the cache is to be regenerated)

	VertexArray *bottom = new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE);
	VertexArray *top = new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE);

	const Color dark(0);
	const Color bright(13, 13, 13, 13);

	//bottom
	float theta;
	for (theta=0.0; theta < 2.f*float(M_PI); theta+=0.1f) {
		bottom->Add(
				vector3f(100.0f*sin(theta), float(-40.0 - 30.0*noise(sin(theta),1.0,cos(theta))), 100.0f*cos(theta)),
				dark);
		bottom->Add(
			vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
			bright);
	}
	theta = 2.f*float(M_PI);
	bottom->Add(
		vector3f(100.0f*sin(theta), float(-40.0 - 30.0*noise(sin(theta),1.0,cos(theta))), 100.0f*cos(theta)),
		dark);
	bottom->Add(
		vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
		bright);
	//top
	for (theta=0; theta < 2.f*float(M_PI); theta+=0.1f) {
		top->Add(
			vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
			bright);
		top->Add(
			vector3f(100.0f*sin(theta), float(40.0 + 30.0*noise(sin(theta),-1.0,cos(theta))), 100.0f*cos(theta)),
			dark);
	}
	theta = 2.f*float(M_PI);
	top->Add(
		vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
		bright);
	top->Add(
		vector3f(100.0f*sin(theta), float(40.0 + 30.0*noise(sin(theta),-1.0,cos(theta))), 100.0f*cos(theta)),
		dark);

	Graphics::MaterialDescriptor desc;
	desc.effect = Graphics::EFFECT_STARFIELD;
	desc.vertexColors = true;
	m_material.Reset(m_renderer->CreateMaterial(desc));
	m_material->emissive = Color::WHITE;
	//This doesn't fade. Could add a generic opacity/intensity value.
	m_model->AddSurface(RefCountedPtr<Surface>(new Surface(TRIANGLE_STRIP, bottom, m_material)));
	m_model->AddSurface(RefCountedPtr<Surface>(new Surface(TRIANGLE_STRIP, top, m_material)));
}

void MilkyWay::Draw(Graphics::RenderState *rs)
{
	assert(m_model != 0);
	m_renderer->DrawStaticMesh(m_model.get(), rs);
}

Container::Container(Graphics::Renderer *renderer, Random &rand)
: m_renderer(renderer)
, m_milkyWay(renderer)
, m_starField(renderer, rand)
, m_universeBox(renderer)
, m_bLoadNewCubemap(true)
, m_uSeed(0)
{
	Graphics::RenderStateDesc rsd;
	rsd.depthTest  = false;
	rsd.depthWrite = false;
	m_renderState = renderer->CreateRenderState(rsd);
	Refresh(rand.Int32());
}

Container::Container(Graphics::Renderer *renderer, Uint32 seed)
: m_renderer(renderer)
, m_milkyWay(renderer)
, m_starField(renderer, Random(seed))
, m_universeBox(renderer)
, m_bLoadNewCubemap(true)
, m_uSeed(seed)
{
	Refresh(seed);
};

void Container::Refresh(Uint32 seed)
{
	// always redo starfield, milkyway stays normal for now
	m_starField.Fill(Random(seed));
	if(m_uSeed != seed) {
		m_bLoadNewCubemap = true;
	}
	m_uSeed = seed;
}

void Container::Draw(const matrix4x4d &transform)
{
	PROFILE_SCOPED()
	m_renderer->SetTransform(transform);

	if(m_bLoadNewCubemap) {
		m_bLoadNewCubemap = false;
		if(Pi::player == nullptr || Pi::player->GetFlightState() != Ship::HYPERSPACE) {
			if(Pi::player && Pi::game->GetSpace()->GetStarSystem()) {
				Uint32 seeds [5];
				const SystemPath& system_path = Pi::game->GetSpace()->GetStarSystem()->GetPath();
				seeds[0] = system_path.systemIndex + 41;
				seeds[1] = system_path.sectorX;
				seeds[2] = system_path.sectorY;
				seeds[3] = system_path.sectorZ;
				seeds[4] = UNIVERSE_SEED;
				Random rand(seeds, 5);
				m_universeBox.LoadCubeMap(&rand);
			} else {
				Random rand(m_uSeed);
				m_universeBox.LoadCubeMap(&rand);
			}
		} else {
			m_universeBox.LoadCubeMap();
		}
	}

	m_renderer->SetTransform(transform);
	m_universeBox.Draw();
	//m_milkyWay.Draw(renderer);
	// squeeze the starfield a bit to get more density near horizon
	matrix4x4d starTrans = transform * matrix4x4d::ScaleMatrix(1.0, 0.4, 1.0);
	m_renderer->SetTransform(starTrans);
	m_starField.Draw();
}

void Container::SetIntensity(float intensity)
{
	PROFILE_SCOPED()
	m_universeBox.SetIntensity(intensity);
	m_starField.SetIntensity(intensity);
	m_milkyWay.SetIntensity(intensity);
}

void Container::SetDrawFlags(const Uint32 flags)
{
	//m_drawFlags = flags;
}

} //namespace Background
