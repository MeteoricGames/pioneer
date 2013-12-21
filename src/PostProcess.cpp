// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "PostProcess.h"
#include "Pi.h"

namespace Graphics {
	namespace GL2 {

PostProcess::PostProcess(const std::string& effect_name, RenderTargetDesc& rtd) :
	strName(effect_name)
{
	mRTDesc.reset(new RenderTargetDesc(
		rtd.width, rtd.height, rtd.colorFormat, rtd.depthFormat, rtd.allowDepthTexture));
}

PostProcess::PostProcess(const std::string& effect_name) :
	strName(effect_name)
{
	WindowSDL* window = Pi::renderer->GetWindow();
	mRTDesc.reset(new RenderTargetDesc(
		window->GetWidth(), window->GetHeight(),
		TextureFormat::TEXTURE_RGB_888, TextureFormat::TEXTURE_NONE,
		false));
}

PostProcess::~PostProcess()
{
	for(unsigned int i = 0; i < vPasses.size(); ++i) {
		delete vPasses[i];
	}
}

void PostProcess::AddPass(const std::string& pass_name, std::shared_ptr<Material>& material)
{
	PostProcessPass* ppp = new PostProcessPass;
	ppp->name = pass_name;
	ppp->material = material;
	ppp->renderTarget.reset(Pi::renderer->CreateRenderTarget(*mRTDesc.get()));
	vPasses.push_back(ppp);
}

	}
}