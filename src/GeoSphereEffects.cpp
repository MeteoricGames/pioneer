// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "GeoSphereEffects.h"
#include "GeoSphere.h"
#include "graphics/gl3/Effect.h"

using namespace Graphics;

namespace Effects {

/////////////////////////////////////// TERRAIN EFFECT
TerrainEffect::TerrainEffect(Renderer* renderer, STerrainOptions& terrain_options)
{
	assert(renderer);
	m_renderer = renderer;
	m_options = terrain_options;

	m_atmosColorId = -1;
	m_geosphereScaleId = -1;
	m_geosphereScaledRadiusId = -1;
	m_geosphereAtmosTopRadId = -1;
	m_geosphereCenterId = -1;
	m_geosphereAtmosFogDensityId = -1;
	m_geosphereAtmosInvScaleHeightId = -1;
	m_shadowsId = -1;
	m_occultedLightId = -1;
	m_shadowCentreXId = -1;
	m_shadowCentreYId = -1;
	m_shadowCentreZId = -1;
	m_lradId = -1;
	m_sdivlradId = -1;

	Init();
}

TerrainEffect::~TerrainEffect()
{
}

void TerrainEffect::Init()
{
	GL3::EffectDescriptor desc;
	// Uniforms
	desc.uniforms.push_back("material.emission"					);
	desc.uniforms.push_back("scene.ambient"						);
	desc.uniforms.push_back("su_ModelViewMatrix"				);
	desc.uniforms.push_back("su_ModelViewProjectionMatrix"		);
	desc.uniforms.push_back("su_NormalMatrix"					);
	desc.uniforms.push_back("invLogZfarPlus1"					);
	// Lights
	desc.uniforms.push_back("u_numLights"						);
	//
	desc.uniforms.push_back("u_atmosColor"						);
	desc.uniforms.push_back("u_geosphereScale"					);
	desc.uniforms.push_back("u_geosphereScaledRadius"			);
	desc.uniforms.push_back("u_geosphereAtmosTopRad"			);
	desc.uniforms.push_back("u_geosphereCenter"					);
	desc.uniforms.push_back("u_geosphereAtmosFogDensity"		);
	desc.uniforms.push_back("u_geosphereAtmosInvScaleHeight"	);
	//
	if(m_options.eclipse) {
		desc.uniforms.push_back("u_shadows"							);
		desc.uniforms.push_back("u_occultedLight"					);
		desc.uniforms.push_back("u_shadowCentreX"					);
		desc.uniforms.push_back("u_shadowCentreY"					);
		desc.uniforms.push_back("u_shadowCentreZ"					);
		desc.uniforms.push_back("u_lrad"							);
		desc.uniforms.push_back("u_sdivlrad"						);
	}
	// Settings
	desc.settings.push_back("LIGHTING");
	if(m_options.atmosphere) {
		desc.settings.push_back("ATMOSPHERE");
	}
	if(m_options.lava) {
		desc.settings.push_back("TERRAIN_WITH_LAVA");
	}
	if(m_options.water) {
		desc.settings.push_back("TERRAIN_WITH_WATER");
	}
	if(m_options.eclipse) {
		desc.settings.push_back("ECLIPSE");
	}
	// Shaders
	desc.vertex_shader = "gl3/geosphere/terrain.vert";
	desc.fragment_shader = "gl3/geosphere/terrain.frag";
	// Effect
	CreateEffect(desc);
	EffectMaterial::Init();

	// Geosphere
	m_atmosColorId						= m_effect->GetUniformID("u_atmosColor"						);
	m_geosphereScaleId					= m_effect->GetUniformID("u_geosphereScale"					);
	m_geosphereScaledRadiusId			= m_effect->GetUniformID("u_geosphereScaledRadius"			);
	m_geosphereAtmosTopRadId			= m_effect->GetUniformID("u_geosphereAtmosTopRad"			);
	m_geosphereCenterId					= m_effect->GetUniformID("u_geosphereCenter"				);
	m_geosphereAtmosFogDensityId		= m_effect->GetUniformID("u_geosphereAtmosFogDensity"		);
	m_geosphereAtmosInvScaleHeightId	= m_effect->GetUniformID("u_geosphereAtmosInvScaleHeight"	);

	if(m_options.eclipse) {
		m_shadowsId							= m_effect->GetUniformID("u_shadows"						);
		m_occultedLightId					= m_effect->GetUniformID("u_occultedLight"					);
		m_shadowCentreXId					= m_effect->GetUniformID("u_shadowCentreX"					);
		m_shadowCentreYId					= m_effect->GetUniformID("u_shadowCentreY"					);
		m_shadowCentreZId					= m_effect->GetUniformID("u_shadowCentreZ"					);
		m_lradId							= m_effect->GetUniformID("u_lrad"							);
		m_sdivlradId						= m_effect->GetUniformID("u_sdivlrad"						);
	}
}

void TerrainEffect::Apply()
{
	EffectMaterial::Apply();

	const GeoSphere::MaterialParameters params = *static_cast<GeoSphere::MaterialParameters*>(this->specialParameter0);
	const SystemBody::AtmosphereParameters ap = params.atmosphere;

	m_effect->GetUniform(m_atmosColorId).Set(ap.atmosCol);
	m_effect->GetUniform(m_geosphereScaleId).Set(ap.scale);
	m_effect->GetUniform(m_geosphereAtmosFogDensityId).Set(ap.atmosDensity);
	m_effect->GetUniform(m_geosphereAtmosInvScaleHeightId).Set(ap.atmosInvScaleHeight);
	m_effect->GetUniform(m_geosphereAtmosTopRadId).Set(ap.atmosRadius);
	m_effect->GetUniform(m_geosphereCenterId).Set(ap.center);
	m_effect->GetUniform(m_geosphereScaledRadiusId).Set(ap.planetRadius / ap.scale);
	m_effect->GetUniform(m_geosphereScaleId).Set(ap.scale);

	if(m_options.eclipse) {
		// we handle up to three shadows at a time
		int occultedLight[3] = { -1, -1, -1 };
		vector3f shadowCentreX;
		vector3f shadowCentreY;
		vector3f shadowCentreZ;
		vector3f srad;
		vector3f lrad;
		vector3f sdivlrad;
		std::vector<Camera::Shadow>::const_iterator it = params.shadows.begin(), itEnd = params.shadows.end();
		int j = 0;
		while (j < 3 && it != itEnd) {
			occultedLight[j] = it->occultedLight;
			shadowCentreX[j] = it->centre[0];
			shadowCentreY[j] = it->centre[1];
			shadowCentreZ[j] = it->centre[2];
			srad[j] = it->srad;
			lrad[j] = it->lrad;
			sdivlrad[j] = it->srad / it->lrad;
			it++;
			j++;
		}
		m_effect->GetUniform(m_shadowsId).Set(j);
		m_effect->GetUniform(m_occultedLightId).Set(occultedLight);
		m_effect->GetUniform(m_shadowCentreXId).Set(shadowCentreX);
		m_effect->GetUniform(m_shadowCentreYId).Set(shadowCentreY);
		m_effect->GetUniform(m_shadowCentreZId).Set(shadowCentreZ);
		m_effect->GetUniform(m_lradId).Set(lrad);
		m_effect->GetUniform(m_sdivlradId).Set(sdivlrad);
	}
}

//////////////////////////////////////////////////// SKYEFFECT
SkyEffect::SkyEffect(Graphics::Renderer* renderer, bool eclipse)
{
	assert(renderer);
	m_renderer = renderer;
	m_eclipse = eclipse;

	m_atmosColorId = -1;
	m_geosphereScaleId = -1;
	m_geosphereScaledRadiusId = -1;
	m_geosphereAtmosTopRadId = -1;
	m_geosphereCenterId = -1;
	m_geosphereAtmosFogDensityId = -1;
	m_geosphereAtmosInvScaleHeightId = -1;
	m_shadowsId = -1;
	m_occultedLightId = -1;
	m_shadowCentreXId = -1;
	m_shadowCentreYId = -1;
	m_shadowCentreZId = -1;
	m_lradId = -1;
	m_sdivlradId = -1;

	Init();
}

SkyEffect::~SkyEffect()
{

}

void SkyEffect::Init()
{
	GL3::EffectDescriptor desc;

	desc.vertex_shader = "gl3/geosphere/sky.vert";
	desc.fragment_shader = "gl3/geosphere/sky.frag";

	desc.uniforms.push_back("su_ModelViewMatrix");
	desc.uniforms.push_back("su_ModelViewProjectionMatrix");
	desc.uniforms.push_back("invLogZfarPlus1");
	//
	desc.uniforms.push_back("u_atmosColor");
	desc.uniforms.push_back("u_geosphereScale");
	desc.uniforms.push_back("u_geosphereScaledRadius");
	desc.uniforms.push_back("u_geosphereAtmosTopRad");
	desc.uniforms.push_back("u_geosphereCenter");
	desc.uniforms.push_back("u_geosphereAtmosFogDensity");
	desc.uniforms.push_back("u_geosphereAtmosInvScaleHeight");
	// Lighting
	desc.settings.push_back("LIGHTING");
	desc.uniforms.push_back("u_numLights");

	if(m_eclipse) {
		desc.settings.push_back("ECLIPSE");
		desc.uniforms.push_back("u_shadows");
		desc.uniforms.push_back("u_occultedLight");
		desc.uniforms.push_back("u_shadowCentreX");
		desc.uniforms.push_back("u_shadowCentreY");
		desc.uniforms.push_back("u_shadowCentreZ");
		desc.uniforms.push_back("u_srad");
		desc.uniforms.push_back("u_lrad");
		desc.uniforms.push_back("u_sdivlrad");
	}

	CreateEffect(desc);
	EffectMaterial::Init();

	m_atmosColorId = m_effect->GetUniformID("u_atmosColor");
	m_geosphereScaleId = m_effect->GetUniformID("u_geosphereScale");
	m_geosphereScaledRadiusId = m_effect->GetUniformID("u_geosphereScaledRadius");
	m_geosphereAtmosTopRadId = m_effect->GetUniformID("u_geosphereAtmosTopRad");
	m_geosphereCenterId = m_effect->GetUniformID("u_geosphereCenter");
	m_geosphereAtmosFogDensityId = m_effect->GetUniformID("u_geosphereAtmosFogDensity");
	m_geosphereAtmosInvScaleHeightId = m_effect->GetUniformID("u_geosphereAtmosInvScaleHeight");

	if(m_eclipse) {
		m_shadowsId = m_effect->GetUniformID("u_shadows");
		m_occultedLightId = m_effect->GetUniformID("u_occultedLight");
		m_shadowCentreXId = m_effect->GetUniformID("u_shadowCentreX");
		m_shadowCentreYId = m_effect->GetUniformID("u_shadowCentreY");
		m_shadowCentreZId = m_effect->GetUniformID("u_shadowCentreZ");
		m_lradId = m_effect->GetUniformID("u_lrad");
		m_sdivlradId = m_effect->GetUniformID("u_sdivlrad");
	}
}

void SkyEffect::Apply()
{
	EffectMaterial::Apply();

	const GeoSphere::MaterialParameters params = *static_cast<GeoSphere::MaterialParameters*>(this->specialParameter0);
	const SystemBody::AtmosphereParameters ap = params.atmosphere;

	m_effect->GetUniform(m_atmosColorId).Set(ap.atmosCol);
	m_effect->GetUniform(m_geosphereScaleId).Set(ap.scale);
	m_effect->GetUniform(m_geosphereAtmosFogDensityId).Set(ap.atmosDensity);
	m_effect->GetUniform(m_geosphereAtmosInvScaleHeightId).Set(ap.atmosInvScaleHeight);
	m_effect->GetUniform(m_geosphereAtmosTopRadId).Set(ap.atmosRadius);
	m_effect->GetUniform(m_geosphereCenterId).Set(ap.center);
	m_effect->GetUniform(m_geosphereScaledRadiusId).Set(ap.planetRadius / ap.scale);

	if (m_eclipse) {
		// we handle up to three shadows at a time
		int occultedLight[3] = { -1, -1, -1 };
		vector3f shadowCentreX;
		vector3f shadowCentreY;
		vector3f shadowCentreZ;
		vector3f srad;
		vector3f lrad;
		vector3f sdivlrad;
		std::vector<Camera::Shadow>::const_iterator it = params.shadows.begin(), itEnd = params.shadows.end();
		int j = 0;
		while (j < 3 && it != itEnd) {
			occultedLight[j] = it->occultedLight;
			shadowCentreX[j] = it->centre[0];
			shadowCentreY[j] = it->centre[1];
			shadowCentreZ[j] = it->centre[2];
			srad[j] = it->srad;
			lrad[j] = it->lrad;
			sdivlrad[j] = it->srad / it->lrad;
			it++;
			j++;
		}
		m_effect->GetUniform(m_shadowsId).Set(j);
		m_effect->GetUniform(m_occultedLightId).Set(occultedLight);
		m_effect->GetUniform(m_shadowCentreXId).Set(shadowCentreX);
		m_effect->GetUniform(m_shadowCentreYId).Set(shadowCentreY);
		m_effect->GetUniform(m_shadowCentreZId).Set(shadowCentreZ);
		m_effect->GetUniform(m_lradId).Set(lrad);
		m_effect->GetUniform(m_sdivlradId).Set(sdivlrad);
	}
}

}
