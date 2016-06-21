// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "PostProcessing.h"
#include "PostProcess.h"
#include "RenderTarget.h"
#include "WindowSDL.h"
#include "Renderer.h"
#include "RenderState.h"
#include "gl3/Effect.h"
#include "gl3/EffectMaterial.h"
#include "gl3/BuiltInShaders.h"
#include "utils.h"

namespace Graphics {

PostProcessing::PostProcessing(Renderer *renderer) :
	m_mtrlFullscreenQuad(nullptr), 
	m_renderer(renderer), 
	m_rtDevice(nullptr),
	m_bPerformPostProcessing(false),
	m_currentLayer(EPP_LAYER_GAME)
{
	assert(m_renderer != nullptr);
	Init();
}

PostProcessing::~PostProcessing()
{

}

void PostProcessing::Init()
{
	// Init postprocessing fullscreen quad effect
	GL3::EffectDescriptorDirect fq_desc;
	fq_desc.vertex_shader_debug_name = "PP->FullscreenQuadVS";
	fq_desc.vertex_shader = GL3::Shaders::FullscreenTexturedQuadVS;
	fq_desc.fragment_shader_debug_name = "PP->FullscreenQuadFS";
	fq_desc.fragment_shader = GL3::Shaders::FullscreenTexturedQuadFS;
	fq_desc.uniforms.push_back("texture0");
	GL3::Effect* fq_effect = new GL3::Effect(m_renderer, fq_desc);
	m_mtrlFullscreenQuad.reset(new GL3::EffectMaterial(m_renderer, fq_effect));

	// Init render targets for both layers
	WindowSDL* window = m_renderer->GetWindow();
	RenderTargetDesc rt_desc(
		window->GetWidth(), window->GetHeight(), 
		TextureFormat::TEXTURE_RGBA_8888, TextureFormat::TEXTURE_DEPTH,
		true);
	m_rtMain = m_renderer->CreateRenderTarget(rt_desc);

	// Init render target for intermediary buffer

	// Init render states
	RenderStateDesc rsd;
	rsd.blendMode = BlendMode::BLEND_SOLID;
	rsd.depthTest = false;
	rsd.depthWrite = false;
	m_renderState = m_renderer->CreateRenderState(rsd);
	rsd.blendMode = BlendMode::BLEND_ALPHA_PREMULT;
	rsd.depthTest = false;
	rsd.depthWrite = false;
	m_renderStateLayer = m_renderer->CreateRenderState(rsd);
}

// rt_device is added so device render target can be changed
void PostProcessing::BeginFrame(PostProcessLayer layer)
{
	if (m_bPerformPostProcessing) {
		m_currentLayer = layer;
		m_renderer->SetRenderTarget(m_rtMain);
		m_renderer->ClearScreen();
	} else {
		m_renderer->SetRenderTarget(m_rtDevice);
	}
}

void PostProcessing::EndFrame()
{
	if(m_bPerformPostProcessing) {
		m_renderer->SetRenderTarget(m_rtDevice);
	}
}

// For post processing material:
// Normal pass: 
//		texture0: set to previous pass output (main if it's the first pass)
// Compose pass: (at least 1 pass before it to work)
//		texture0: set to main output always
//		texture1: set to previous output
void PostProcessing::Run(PostProcess* pp)
{
	if (!m_bPerformPostProcessing) {
		return;
	}
	const int layer = static_cast<int>(m_currentLayer);
	// Check whether all passes are bypassed = no post processing
	bool all_bypass = true;
	for(unsigned int i = 0; i < pp->vPasses.size(); ++i) {
		if(!pp->vPasses[i]->bypass) {
			all_bypass = false;
			break;
		}
	}
	// Perform post processing
	if(pp == nullptr || pp->GetPassCount() == 0 || all_bypass) {
		// No post-processing
		m_renderer->SetRenderTarget(m_rtDevice);
		m_mtrlFullscreenQuad->texture0 = m_rtMain->GetColorTexture();
		if(layer == 0) {
			m_renderer->DrawFullscreenQuad(m_mtrlFullscreenQuad.get(), m_renderState, true);
		} else {
			m_renderer->DrawFullscreenQuad(m_mtrlFullscreenQuad.get(), m_renderStateLayer, false);
		}
	} else {
		RenderTarget* rt_src = m_rtMain;
		RenderTarget* rt_dest = m_rtDevice;
		RenderState* rstate = m_renderState;
		bool clear_rt = true;
		for(unsigned int i = 0; i < pp->vPasses.size(); ++i) {
			if(pp->vPasses[i]->bypass && i > 0 && i < pp->vPasses.size() - 1) {
				// Bypass pass, move to the next one.
				continue;
			}
			if(i == pp->vPasses.size() - 1) { // Last pass
				rt_dest = m_rtDevice;
			} else {
				rt_dest = pp->vPasses[i]->renderTarget.get();
			}
			if (layer > 0) {
				rstate = m_renderStateLayer;
				clear_rt = false;
			}
			m_renderer->SetRenderTarget(rt_dest);

			if(pp->vPasses[i]->effect_type == PostProcessEffectType::PP_ET_MATERIAL) { 
				// Legacy materials
				Material *mtrl = pp->vPasses[i]->material.get();
				if(pp->vPasses[i]->type == PP_PASS_COMPOSE) {
					mtrl->texture0 = m_rtMain->GetColorTexture();
					mtrl->texture1 = rt_src->GetColorTexture();
				} else {
					mtrl->texture0 = rt_src->GetColorTexture();
				}
				m_renderer->DrawFullscreenQuad(mtrl, rstate, clear_rt);
			} else { 
				// OpenGL 3 Effects
				m_renderer->SetEffect(pp->vPasses[i]->effect.get());
				pp->vPasses[i]->effect->SetProgram();
				if(pp->vPasses[i]->type == PP_PASS_COMPOSE) {
					pp->vPasses[i]->effect->GetUniform(pp->vPasses[i]->texture0Id).Set(
						m_rtMain->GetColorTexture(), 0);
					pp->vPasses[i]->effect->GetUniform(pp->vPasses[i]->texture1Id).Set(
						rt_src->GetColorTexture(), 1);
				} else {
					pp->vPasses[i]->effect->GetUniform(pp->vPasses[i]->texture0Id).Set(
						rt_src->GetColorTexture(), 0);
				}
				m_renderer->DrawFullscreenQuad(rstate, clear_rt);
			}
			rt_src = rt_dest;
		}
	}
}

void PostProcessing::SetPerformPostProcessing(bool enabled) 
{
	if(m_bPerformPostProcessing != enabled) {
		m_bPerformPostProcessing = enabled;
		if(enabled) {
			// TODO: Reinitialize all render target buffers?
		} else {
			// TODO: Release all render target buffers?
		}
	}
}

void PostProcessing::SetDeviceRT(RenderTarget* rt_device)
{
	m_rtDevice = rt_device;
}

}
