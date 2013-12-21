// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "PostProcessing.h"
#include "Pi.h"
#include "graphics/RenderTarget.h"

namespace Graphics {
	namespace GL2 {

PostProcessing::PostProcessing() :
	mtrlFullscreenQuad(nullptr)
{
	Init();
}

PostProcessing::~PostProcessing()
{

}

void PostProcessing::Run(PostProcess* pp)
{
	if(pp == nullptr) return;
}

void PostProcessing::Init()
{
	// Init quad used for rendering
	const float screenquad_vertices [] = {
		-1.0f,	-1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
		-1.0f,	 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
		 1.0f,	 1.0f, 0.0f,
		-1.0f,   1.0f, 0.0f
	};
	glGenBuffers(1, &uScreenQuadBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, uScreenQuadBufferId);
	glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), screenquad_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Init postprocessing materials
	MaterialDescriptor tfquad_mtrl_desc;
	tfquad_mtrl_desc.effect = EFFECT_TEXTURED_FULLSCREEN_QUAD;
	mtrlFullscreenQuad.reset(Pi::renderer->CreateMaterial(tfquad_mtrl_desc));

	// Init render targets
	WindowSDL* window = Pi::renderer->GetWindow();
	RenderTargetDesc rt_desc(
		window->GetWidth(), window->GetHeight(), 
		TextureFormat::TEXTURE_RGBA_8888, TextureFormat::TEXTURE_DEPTH,
		true);
	rtMain = Pi::renderer->CreateRenderTarget(rt_desc);
}

	}
}
