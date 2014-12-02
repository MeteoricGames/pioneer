// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "SMAA.h"
#include "graphics/RendererGL2.h"
#include "graphics/PostProcess.h"
#include "graphics/gl3/Effect.h"
#include "utils.h"
#include "StringF.h"
#include "graphics/TextureBuilder.h"
#include "graphics/Texture.h"

SMAA::SMAA(Graphics::Renderer* renderer) :
	m_renderer(renderer)
{
	assert(m_renderer);
	Init();
}

SMAA::~SMAA()
{

}

void SMAA::Init()
{
	// SMAA static textures
	m_areaTexture =
		Graphics::TextureBuilder::UI("textures/smaa/areaTex.png").GetOrCreateTexture(m_renderer, "effect");
	m_searchTexture = 
		Graphics::TextureBuilder::UI("textures/smaa/searchTex.png").GetOrCreateTexture(m_renderer, "effect");

	// SMAA post process
	m_postprocess.reset(new Graphics::PostProcess("SMAA", m_renderer->GetWindow(), true));
	
	const std::string pixel_size = stringf<float, float>(
		"SMAA_PIXEL_SIZE float2(1.0 / %0, 1.0 / %1)",
		static_cast<float>(m_renderer->GetWindow()->GetWidth()),
		static_cast<float>(m_renderer->GetWindow()->GetHeight()));
	const std::string quality_preset = "SMAA_PRESET_ULTRA 1";

	{
		// Pass 1: Luma edge detection
		Graphics::GL3::EffectDescriptor ed;
		ed.version = Graphics::GL3::EffectGLSLVersion::ESV_GLSL_120;
		ed.settings.push_back(pixel_size);
		ed.settings.push_back(quality_preset);
		//ed.header_shaders.push_back("smaa/smaa_gl2.h");
		ed.vertex_shader = "smaa/smaa_LumaEdgeDetection.vert";
		ed.fragment_shader = "smaa/smaa_LumaEdgeDetection.frag";
		ed.uniforms.push_back("colorTex");
		m_pass1.reset(new Graphics::GL3::Effect(m_renderer, ed));
		m_postprocess->AddPass(m_renderer, 
			"LumaEdgeDetection", m_pass1);
	}
	{
		// Pass 2: Blending weight calculation
		Graphics::GL3::EffectDescriptor ed;
		ed.version = Graphics::GL3::EffectGLSLVersion::ESV_GLSL_120;
		ed.settings.push_back(pixel_size);
		ed.settings.push_back(quality_preset);
		//ed.header_shaders.push_back("smaa/smaa_gl2.h");
		ed.vertex_shader = "smaa/smaa_BlendingWeightCalculation.vert";
		ed.fragment_shader = "smaa/smaa_BlendingWeightCalculation.frag";
		ed.uniforms.push_back("edgesTex");
		ed.uniforms.push_back("areaTex");
		ed.uniforms.push_back("searchTex");
		m_pass2.reset(new Graphics::GL3::Effect(m_renderer, ed));
		m_postprocess->AddPass(m_renderer, 
			"BlendingWeightCalculation", m_pass2);
		m_areaTexUniformID = m_pass2->GetUniformID("areaTex");
		m_searchTexUniformID = m_pass2->GetUniformID("searchTex");
	}
	{
		// Pass 3: Neighborhood blending
		Graphics::GL3::EffectDescriptor ed;
		ed.version = Graphics::GL3::EffectGLSLVersion::ESV_GLSL_120;
		ed.settings.push_back(pixel_size);
		ed.settings.push_back(quality_preset);
		//ed.header_shaders.push_back("smaa/smaa_gl2.h");
		ed.vertex_shader = "smaa/smaa_NeighborhoodBlending.vert";
		ed.fragment_shader = "smaa/smaa_NeighborhoodBlending.frag";
		ed.uniforms.push_back("colorTex");
		ed.uniforms.push_back("blendTex");
		m_pass3.reset(new Graphics::GL3::Effect(m_renderer, ed));
		m_postprocess->AddPass(m_renderer, 
			"NeighborhoodBlending", m_pass3, Graphics::PostProcessPassType::PP_PASS_COMPOSE);
		//m_p3ColorTexUniformID = m_pass3->GetUniformID("colorTex");
	}
}

void SMAA::PostProcess()
{
	// Update uniforms
	m_pass2->SetProgram();
	m_pass2->GetUniform(m_areaTexUniformID).Set(m_areaTexture, 1);
	m_pass2->GetUniform(m_searchTexUniformID).Set(m_searchTexture, 2);
	// Execute post processing
	m_renderer->PostProcessFrame(m_postprocess.get());
}
