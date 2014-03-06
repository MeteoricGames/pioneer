// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _BACKGROUND_H
#define _BACKGROUND_H

#include "libs.h"
#include "galaxy/SystemPath.h"
#include "graphics/Texture.h"
#include "graphics/RenderState.h"
#include "Random.h"

namespace Graphics {
	class Renderer;
	class StaticMesh;
	class Material;
}

/*
 * Classes to draw background stars and the milky way
 */

namespace Background
{
	class BackgroundElement
	{
	public:
		virtual void SetIntensity(float intensity);

	protected:
		Graphics::Renderer *m_renderer;
		RefCountedPtr<Graphics::Material> m_material;
	};

	class UniverseBox : public BackgroundElement
	{
	public:
		UniverseBox(Graphics::Renderer *r);
		~UniverseBox();

		void Draw();
		void LoadCubeMap(Random* randomizer = nullptr);

		virtual void SetIntensity(float intensity) override;

	private:
		void Init();
		Random createRandom(Uint32 seed);
		Random createRandom(const SystemPath& system_path);

		Graphics::StaticMesh *m_model;
		Graphics::Texture* m_cubemap;
		float fIntensity;
		Graphics::RenderState* m_cubeRS;
	};

	class Starfield : public BackgroundElement
	{
	public:
		//does not Fill the starfield
		Starfield(Graphics::Renderer *r, Random &rand);
		void Draw();
		//create or recreate the starfield
		void Fill(Random &rand);

	private:
		void Init();
		static const int BG_STAR_MAX = 10000;
		std::unique_ptr<Graphics::StaticMesh> m_model;

		//hyperspace animation vertex data
		vector3f m_hyperVtx[BG_STAR_MAX*2];
		Color m_hyperCol[BG_STAR_MAX*2];
		Graphics::RenderState* m_starfieldRS;
	};

	class MilkyWay : public BackgroundElement
	{
	public:
		MilkyWay(Graphics::Renderer*);
		void Draw(Graphics::RenderState*);

	private:
		std::unique_ptr<Graphics::StaticMesh> m_model;
	};



	// contains starfield, milkyway, possibly other Background elements
	class Container
	{
	public:

		// default constructor, needs Refresh with proper seed to show starfield
		Container(Graphics::Renderer*, Random &rand);
		Container(Graphics::Renderer*, Uint32 seed);
		void Draw(const matrix4x4d &transform);
		void Refresh(Uint32 seed);
		void Refresh(Random &rand);

		void SetIntensity(float intensity);
		void SetDrawFlags(const Uint32 flags);

	private:
		Graphics::Renderer *m_renderer;
		std::shared_ptr<MilkyWay> m_milkyWay;
		std::shared_ptr<Starfield> m_starField;
		std::shared_ptr<UniverseBox> m_universeBox;
		bool m_bLoadNewCubemap;
		Uint32 m_uSeed;
		Graphics::RenderState *m_renderState;
	};

} //namespace Background

#endif
