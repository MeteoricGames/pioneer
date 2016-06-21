// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "MainMaterial.h"
#include "StringF.h"
#include "Shields.h"

using namespace Graphics;
using namespace Graphics::GL3;

MainMaterial::MainMaterial(Graphics::Renderer* renderer, Graphics::MaterialDescriptor& material_desc, 
	bool lit) : m_shieldStrengthId(-1), m_shieldCooldownId(-1), m_hitPosId(-1), m_radiiId(-1), m_numHitsId(-1)
{
	m_descriptor = material_desc;
	m_renderer = renderer;
	m_effect = nullptr;
	m_isLit = lit || material_desc.lighting;
	assert(renderer && (material_desc.effect == EffectType::EFFECT_DEFAULT ||
		material_desc.effect == EffectType::EFFECT_SHIELD));
	Init();
}

MainMaterial::~MainMaterial()
{

}

void MainMaterial::Init()
{
	EffectDescriptor e_desc;
	if(m_descriptor.effect == EffectType::EFFECT_SHIELD) {
		e_desc.settings.push_back("SHIELD_SHADER");
	}
	if(m_isLit) { 
		e_desc.settings.push_back("LIGHTING");
	}
	if(m_descriptor.textures > 0) {
		e_desc.settings.push_back("TEXTURE0");
	}
	if(m_descriptor.vertexColors) {
		e_desc.settings.push_back("VERTEXCOLOR");
	}
	if(m_descriptor.alphaTest) {
		e_desc.settings.push_back("ALPHA_TEST");
	}
	if(m_descriptor.pointsMode) {
		e_desc.settings.push_back("POINTS");
	}
	if(m_descriptor.colorTint) {
		e_desc.settings.push_back("COLOR_TINT");
	}
	if(m_descriptor.specularMap) {
		e_desc.settings.push_back("MAP_SPECULAR");
	}
	if(m_descriptor.glowMap) {
		e_desc.settings.push_back("MAP_EMISSIVE");
	}
	if(m_descriptor.usePatterns) {
		e_desc.settings.push_back("MAP_COLOR");
	}
	if(m_descriptor.quality & HAS_HEAT_GRADIENT) {
		e_desc.settings.push_back("HEAT_COLOURING");
	}
	if(m_descriptor.testMode) {
		e_desc.settings.push_back("TEST_MODE");
	}
	e_desc.uniforms = {
		"invLogZfarPlus1",
		"texture0",
		"texture1",
		"texture2",
		"texture3",
		"texture4",
		"heatGradient",
		"heatingMatrix",
		"heatingNormal",
		"heatingAmount",
		"scene.ambient",
		"pointSize",
		"colorTint",
		"su_ModelViewMatrix",
		"su_NormalMatrix",
		"su_ModelViewProjectionMatrix",
		"u_numLights",
		"su_ViewMatrixInverse",
	};
	if (m_isLit && m_descriptor.irradiance) {
		e_desc.uniforms.push_back("u_universeBox");
		e_desc.uniforms.push_back("u_atmosDensity");
		e_desc.uniforms.push_back("u_atmosColor");
		e_desc.uniforms.push_back("u_hemiDiff");
		e_desc.uniforms.push_back("u_hemiGloss");
		e_desc.uniforms.push_back("u_irrGlossyExp");
		e_desc.settings.push_back("HEMISPHERE_LIGHT");
	}
	if (m_descriptor.effect == EffectType::EFFECT_SHIELD) {
		e_desc.uniforms.push_back("varyingEyepos");
		e_desc.uniforms.push_back("varyingNormal");
		e_desc.uniforms.push_back("varyingVertex");
		e_desc.uniforms.push_back("shieldStrength");
		e_desc.uniforms.push_back("shieldCooldown");
		e_desc.uniforms.push_back("hitPos");
		e_desc.uniforms.push_back("radii");
		e_desc.uniforms.push_back("numHits");
	}

	e_desc.vertex_shader = "gl3/multi.vert";
	e_desc.fragment_shader = "gl3/multi.frag";	

	m_effect = new Effect(m_renderer, e_desc);
	EffectMaterial::Init();

	assert(m_effect->GetAttribute(EEffectAttributes::EEA_POSITION).GetLocation() != -1);
	//assert(m_effect->GetAttribute(EEffectAttributes::EEA_NORMAL).GetLocation() != -1);

	if(m_descriptor.effect == EffectType::EFFECT_SHIELD) {
		m_shieldStrengthId = m_effect->GetUniformID("shieldStrength");
		m_shieldCooldownId = m_effect->GetUniformID("shieldCooldown");
		m_hitPosId = m_effect->GetUniformID("hitPos");
		m_radiiId = m_effect->GetUniformID("radii");
		m_numHitsId = m_effect->GetUniformID("numHits");
		assert(m_shieldStrengthId + m_shieldCooldownId + m_hitPosId + m_radiiId + m_numHitsId > -5);
	}
}

void MainMaterial::Apply()
{
	EffectMaterial::Apply();
	if(m_descriptor.effect == EffectType::EFFECT_SHIELD) {
		if(specialParameter0) {
			const ShieldRenderParameters srp = *static_cast<ShieldRenderParameters*>(this->specialParameter0);
			if(m_shieldStrengthId > -1) {
				m_effect->GetUniform(m_shieldStrengthId).Set(srp.strength);
			}
			if(m_shieldCooldownId > -1) {
				m_effect->GetUniform(m_shieldCooldownId).Set(srp.coolDown);
			}
			if(m_hitPosId > -1) {
				m_effect->GetUniform(m_hitPosId).Set(srp.hitPos, MAX_SHIELD_HITS);
			}
			if(m_radiiId > -1) {
				m_effect->GetUniform(m_radiiId).Set(srp.radii, MAX_SHIELD_HITS);
			}
			if(m_numHitsId > -1) {
				m_effect->GetUniform(m_numHitsId).Set(int(std::min(srp.numHits, MAX_SHIELD_HITS)));
			}
		} else {
			if (m_shieldStrengthId > -1) {
				m_effect->GetUniform(m_shieldStrengthId).Set(0.0f);
			}
			if (m_shieldCooldownId > -1) {
				m_effect->GetUniform(m_shieldCooldownId).Set(0.0f);
			}
			if (m_numHitsId > -1) {
				m_effect->GetUniform(m_numHitsId).Set(0);
			}
		}
	}
}