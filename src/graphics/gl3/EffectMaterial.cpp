// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "EffectMaterial.h"
#include "graphics/RendererGL3.h"
#include "graphics/gl3/Effect.h"
#include "graphics/TextureGL.h"
#include "graphics/gl2/Program.h"
#include "StringF.h"
#include "Pi.h"
#include "Game.h"

namespace Graphics { namespace GL3 {

EffectMaterial::EffectMaterial() : 
	m_renderer(nullptr), m_effect(nullptr)
{
	PreInit();
	// Inheriting class must call Init itself
}

EffectMaterial::EffectMaterial(Renderer* renderer, Effect* effect) :
	m_renderer(renderer), m_effect(effect)
{
	PreInit();
	Init();
}

EffectMaterial::EffectMaterial(Renderer* renderer, EffectDescriptor& effect_desc) :
	m_renderer(renderer), m_effect(nullptr)
{
	PreInit();
	CreateEffect(effect_desc);
	Init();
}

EffectMaterial::EffectMaterial(Renderer* renderer, EffectDescriptorDirect& effect_desc) :
	m_renderer(renderer), m_effect(nullptr)
{
	PreInit();
	CreateEffect(effect_desc);
	Init();
}

EffectMaterial::~EffectMaterial()
{
	m_effect->DecRefCount();
}

void EffectMaterial::PreInit()
{
	m_usesDeprecatedMtrl = false;
	m_invLogZfarPlus1Id = -1;

	m_numLightsId = -1;
	m_lightsBlock = -1;
	m_materialsBlock = -1;

	m_diffuseId = -1;
	m_specularId = -1;
	m_emissiveId = -1;
	m_shininessId = -1;

	m_texture0Id = -1;
	m_texture1Id = -1;
	m_texture2Id = -1;
	m_texture3Id = -1;
	m_texture4Id = -1;
	m_heatGradId = -1;
	m_heatMatId = -1;
	m_heatNormId = -1;
	m_heatAmountId = -1;
	m_ambientId = -1;
	m_tintId = -1;
	m_pointSizeId = -1;
	m_universeBoxId = -1;
	m_atmosColorId = -1;
	m_atmosDensityId = -1;

	m_hemiDiffId = -1;
	m_hemiGlossId = -1;
	m_irrGlossyExpId = -1;

	m_isInit = false;
}

void EffectMaterial::Init()
{
	assert(m_renderer && m_effect);
	m_effect->IncRefCount();

	m_invLogZfarPlus1Id = m_effect->GetUniformID("invLogZfarPlus1");

	// Light block ("UBLightSources")
	m_numLightsId = m_effect->GetUniformID("u_numLights");
	if(m_numLightsId > -1) {
		m_lightsBlock = m_effect->InitUniformBlock(Effect::GetSharedUniformBlock(
			LightSourcesBlock_Name.c_str()));
	}

	// Lighting tweaks
	m_hemiDiffId = m_effect->GetUniformID("u_hemiDiff");
	m_hemiGlossId = m_effect->GetUniformID("u_hemiGloss");
	m_irrGlossyExpId = m_effect->GetUniformID("u_irrGlossyExp");
	
	// Textures
	m_texture0Id = m_effect->GetUniformID("texture0");
	m_texture1Id = m_effect->GetUniformID("texture1");
	m_texture2Id = m_effect->GetUniformID("texture2");
	m_texture3Id = m_effect->GetUniformID("texture3");
	m_texture4Id = m_effect->GetUniformID("texture4");

	// Heat
	m_heatGradId = m_effect->GetUniformID("heatGradient");
	if (m_heatGradId > -1) {
		m_heatMatId = m_effect->GetUniformID("heatingMatrix");
		m_heatNormId = m_effect->GetUniformID("heatingNormal");
		m_heatAmountId = m_effect->GetUniformID("heatingAmount");
	}
	
	// Material block
	m_materialsBlock = m_effect->InitUniformBlock(Effect::GetSharedUniformBlock(
		MaterialBlock_Name.c_str()));

	// Legacy material
	m_diffuseId = m_effect->GetUniformID("material.diffuse");
	m_specularId = m_effect->GetUniformID("material.specular");
	m_emissiveId = m_effect->GetUniformID("material.emission");
	m_shininessId = m_effect->GetUniformID("material.shininess");

	// Other properties
	m_ambientId = m_effect->GetUniformID("scene.ambient");
	m_tintId = m_effect->GetUniformID("colorTint");
	m_pointSizeId = m_effect->GetUniformID("pointSize");
	if (m_texture0Id + m_texture1Id + m_texture2Id + m_texture3Id + m_texture4Id == -5) {
		m_usesDefaultTextures = false;
	} else {
		m_usesDefaultTextures = true;
	}
	if (m_heatGradId + m_diffuseId + m_specularId + m_emissiveId + m_shininessId + m_tintId + m_pointSizeId == -7) {
		m_usesDeprecatedMtrl = false;
	} else {
		m_usesDeprecatedMtrl = true;
	}
	m_universeBoxId = m_effect->GetUniformID("u_universeBox");
	if(m_universeBoxId > -1) {
		m_atmosColorId = m_effect->GetUniformID("u_atmosColor");
		m_atmosDensityId = m_effect->GetUniformID("u_atmosDensity");
	}
	m_isInit = true;
}

void EffectMaterial::CreateEffect(EffectDescriptor& effect_desc)
{
	m_effect = new Effect(m_renderer, effect_desc);
}

void EffectMaterial::CreateEffect(EffectDescriptorDirect& effect_desc)
{
	m_effect = new Effect(m_renderer, effect_desc);
}

void EffectMaterial::Apply()
{
	assert(m_isInit);
	unsigned texcount = 0;
	m_effect->Apply();
	if(m_invLogZfarPlus1Id > -1) {
		m_effect->GetUniform(m_invLogZfarPlus1Id).Set(m_renderer->GetInvLogZfarPlus1());
	}
	if(m_usesDefaultTextures) {
		if (m_texture0Id > -1) {
			m_effect->GetUniform(m_texture0Id).Set(texture0, texcount++);
		}
		if (m_texture1Id > -1) {
			m_effect->GetUniform(m_texture1Id).Set(texture1, texcount++);
		}
		if (m_texture2Id > -1) {
			m_effect->GetUniform(m_texture2Id).Set(texture2, texcount++);
		}
		if (m_texture3Id > -1) {
			m_effect->GetUniform(m_texture3Id).Set(texture3, texcount++);
		}
		if (m_texture4Id > -1) {
			m_effect->GetUniform(m_texture4Id).Set(texture4, texcount++);
		}
	}
	if(m_numLightsId > -1 && m_renderer->GetNumDirLights() > 0) {
		m_effect->GetUniform(m_numLightsId).Set(static_cast<float>(m_renderer->GetNumDirLights()));
		m_effect->WriteUniformBlock(m_lightsBlock, sizeof(LightSource) * MATERIAL_MAX_LIGHTS, 
			m_renderer->GetLightSources());
	}
	if(m_materialsBlock > -1) {
		MaterialBlock mb(diffuse, emissive, specular, shininess);
		m_effect->WriteUniformBlock(m_materialsBlock, sizeof(MaterialBlock), &mb);
	}
	if(m_hemiDiffId > -1) {
		m_effect->GetUniform(m_hemiDiffId).Set(Pi::ts_irr_light.hemi_diffuse_intensity);
		m_effect->GetUniform(m_hemiGlossId).Set(Pi::ts_irr_light.hemi_gloss_intensity);
		m_effect->GetUniform(m_irrGlossyExpId).Set(Pi::ts_irr_light.irradiance_gloss_exponential);
	}

	if(m_usesDeprecatedMtrl) {
		if(m_heatGradId > -1) {
			m_effect->GetUniform(m_heatGradId).Set(heatGradient, texcount++);
			if(specialParameter0 != nullptr) {
				HeatGradientParameters_t *pMGP = static_cast<HeatGradientParameters_t*>(specialParameter0);
				if(m_heatMatId > -1) {
					m_effect->GetUniform(m_heatMatId).Set(pMGP->heatingMatrix);
				}
				if(m_heatNormId > -1) {
					m_effect->GetUniform(m_heatNormId).Set(pMGP->heatingNormal);
				}
				if(m_heatAmountId > -1) {
					m_effect->GetUniform(m_heatAmountId).Set(pMGP->heatingAmount);
				}
			}
		}
		if (m_diffuseId > -1) {
			m_effect->GetUniform(m_diffuseId).Set(diffuse);
		}
		if (m_specularId > -1) {
			m_effect->GetUniform(m_specularId).Set(specular);
		}
		if (m_emissiveId > -1) {
			m_effect->GetUniform(m_emissiveId).Set(emissive);
		}
		if (m_shininessId > -1) {
			m_effect->GetUniform(m_shininessId).Set(shininess);
		}
		if(m_tintId > -1) {
			m_effect->GetUniform(m_tintId).Set(tint);
		}
		if(m_pointSizeId > -1) {
			m_effect->GetUniform(m_pointSizeId).Set(pointSize);
		}
	}
	if(m_ambientId > -1) {
		m_effect->GetUniform(m_ambientId).Set(m_renderer->GetAmbientColor());
	}
	if (m_universeBoxId > -1) {
		m_effect->GetUniform(m_universeBoxId).Set(Background::UniverseBox::s_cubeMap, texcount++);
		m_effect->GetUniform(m_atmosColorId).Set(atmosphereColor);
		m_effect->GetUniform(m_atmosDensityId).Set(atmosphereDensity);
	}
}

void EffectMaterial::Unapply()
{
	assert(m_effect->IsApplied());
	// Might not be necessary to unbind textures, but let's not old graphics code (eg, old-UI)
	if (heatGradient) {
		glActiveTexture(GL_TEXTURE4);
		static_cast<TextureGL*>(heatGradient)->Unbind();
	}
	if (texture4) {
		glActiveTexture(GL_TEXTURE3);
		static_cast<TextureGL*>(texture4)->Unbind();
	}
	if (texture3) {
		glActiveTexture(GL_TEXTURE2);
		static_cast<TextureGL*>(texture3)->Unbind();
	}
	if (texture2) {
		glActiveTexture(GL_TEXTURE1);
		static_cast<TextureGL*>(texture2)->Unbind();
	}
	if (texture1) {
		glActiveTexture(GL_TEXTURE0);
		static_cast<TextureGL*>(texture1)->Unbind();
	}
	if (texture0) {
		static_cast<TextureGL*>(texture0)->Unbind();
	}
	m_effect->Unapply();
}

}}
