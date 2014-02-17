// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "PostProcessing.h"
#include "PostProcess.h"
#include "RenderTarget.h"
#include "WindowSDL.h"
#include "Renderer.h"

namespace Graphics {

PostProcessing::PostProcessing(Renderer *renderer) :
	mtrlFullscreenQuad(nullptr), mRenderer(renderer), rtDevice(nullptr)
{
	assert(mRenderer != nullptr);
	Init();
}

PostProcessing::~PostProcessing()
{

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
	mtrlFullscreenQuad.reset(mRenderer->CreateMaterial(tfquad_mtrl_desc));

	// Init render targets
	WindowSDL* window = mRenderer->GetWindow();
	RenderTargetDesc rt_desc(
		window->GetWidth(), window->GetHeight(), 
		TextureFormat::TEXTURE_RGBA_8888, TextureFormat::TEXTURE_DEPTH,
		true);
	rtMain = mRenderer->CreateRenderTarget(rt_desc);
}

// rt_device is added so device render target can be changed
void PostProcessing::BeginFrame()
{
	if(bPerformPostProcessing) {
		mRenderer->SetRenderTarget(rtMain);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		mRenderer->SetRenderTarget(rtDevice);
	}
}

void PostProcessing::EndFrame()
{
	if(bPerformPostProcessing) {
		mRenderer->SetRenderTarget(rtDevice);
	}
}

void PostProcessing::Run(PostProcess* pp)
{
	if(!bPerformPostProcessing) return;
	glBindBuffer(GL_ARRAY_BUFFER, uScreenQuadBufferId);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void*>(0));
	if(pp == nullptr || pp->GetPassCount() == 0) {
		// No post-processing
		mRenderer->SetRenderTarget(rtDevice);
		mtrlFullscreenQuad->texture0 = rtMain->GetColorTexture();
		mtrlFullscreenQuad->Apply();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		mtrlFullscreenQuad->Unapply();
	} else {
		RenderTarget* rt_src = rtMain;
		RenderTarget* rt_dest = 0;
		for(unsigned int i = 0; i < pp->vPasses.size(); ++i) {
			if(i == pp->vPasses.size() - 1) {
				rt_dest = 0;
			} else {
				rt_dest = pp->vPasses[i]->renderTarget.get();
			}
			Material *mtrl = pp->vPasses[i]->material.get();
			mRenderer->SetRenderTarget(rt_dest);
			glClear(GL_COLOR_BUFFER_BIT);
			if(pp->vPasses[i]->type == PP_PASS_COMPOSE) {
				mtrl->texture0 = rtMain->GetColorTexture();
				mtrl->texture1 = rt_src->GetColorTexture();
			} else {
				mtrl->texture0 = rt_src->GetColorTexture();
			}
			mtrl->Apply();
			glDrawArrays(GL_TRIANGLES, 0, 6);
			mtrl->Unapply();
			rt_src = rt_dest;
		}
	}
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PostProcessing::SetEnabled(bool enabled) 
{
	if(bPerformPostProcessing != enabled) {
		bPerformPostProcessing = enabled;
		if(enabled) {
			// TODO: Reinitialize all render target buffers
		} else {
			// TODO: Release all render target buffers
		}
	}
}

void PostProcessing::SetDeviceRT(RenderTarget* rt_device)
{
	rtDevice = rt_device;
}

}
