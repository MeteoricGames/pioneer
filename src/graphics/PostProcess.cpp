// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "PostProcess.h"
#include "Renderer.h"
#include "WindowSDL.h"
#include "Material.h"
#include "gl3/Effect.h"

using namespace Graphics::GL3;

namespace Graphics {

PostProcess::PostProcess(const std::string& effect_name, RenderTargetDesc& rtd) :
	strName(effect_name)
{
	mRTDesc.reset(new RenderTargetDesc(
		rtd.width, rtd.height, rtd.colorFormat, rtd.depthFormat, rtd.allowDepthTexture));
}

PostProcess::PostProcess(const std::string& effect_name, WindowSDL* window, bool with_alpha) :
	strName(effect_name)
{
	assert(window != nullptr);
	if(!with_alpha) {
		mRTDesc.reset(new RenderTargetDesc(
			window->GetWidth(), window->GetHeight(),
			TextureFormat::TEXTURE_RGB_888, TextureFormat::TEXTURE_NONE,
			false));
	} else {
		mRTDesc.reset(new RenderTargetDesc(
			window->GetWidth(), window->GetHeight(),
			TextureFormat::TEXTURE_RGBA_8888, TextureFormat::TEXTURE_NONE,
			false));
	}
}

PostProcess::~PostProcess()
{
	for(unsigned int i = 0; i < vPasses.size(); ++i) {
		delete vPasses[i];
	}
}

void PostProcess::AddPass(Renderer* renderer, const std::string& pass_name, 
	std::shared_ptr<Material>& material, PostProcessPassType pass_type)
{
	assert(renderer != nullptr);
	PostProcessPass* ppp = new PostProcessPass;
	ppp->name = pass_name;
	ppp->material = material;
	ppp->type = pass_type;
	ppp->texture0Id = ppp->material->GetEffect()->GetUniformID("texture0");
	assert(ppp->texture0Id != -1);
	if(pass_type == PostProcessPassType::PP_PASS_COMPOSE) {
		ppp->texture1Id = ppp->material->GetEffect()->GetUniformID("texture1");
		assert(ppp->texture1Id != -1);
	}
	if(vPasses.size() > 0) { // Pass creates rendertarget for pass before it, last pass never requires a render target
		vPasses.back()->renderTarget.reset(renderer->CreateRenderTarget(*mRTDesc.get()));
	}
	vPasses.push_back(ppp);
}

void PostProcess::AddPass(Renderer* renderer, const std::string& pass_name, Graphics::EffectType effect_type, PostProcessPassType pass_type)
{
	assert(renderer != nullptr);
	MaterialDescriptor md;
	md.effect = effect_type;
	std::shared_ptr<Material> mtrl;
	mtrl.reset(renderer->CreateMaterial(md));
	return AddPass(renderer, pass_name, mtrl, pass_type);
}

void PostProcess::AddPass(Renderer* renderer, const std::string& pass_name,
	std::shared_ptr<Effect> effect, PostProcessPassType pass_type)
{
	assert(renderer != nullptr);
	PostProcessPass* ppp = new PostProcessPass;
	ppp->name = pass_name;
	ppp->effect = effect;
	ppp->type = pass_type;
	ppp->effect_type = PostProcessEffectType::PP_ET_EFFECT;
	ppp->texture0Id = ppp->effect->GetUniformID("texture0");
	if (pass_type == PostProcessPassType::PP_PASS_COMPOSE) {
		ppp->texture1Id = ppp->effect->GetUniformID("texture1");
	}
	if(vPasses.size() > 0) { // Pass creates rendertarget for pass before it, last pass never requires a render target
		vPasses.back()->renderTarget.reset(renderer->CreateRenderTarget(*mRTDesc.get()));
	}
	vPasses.push_back(ppp);
}

}