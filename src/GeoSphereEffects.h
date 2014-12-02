// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GEOSPHERE_EFFECTS_H_
#define _GEOSPHERE_EFFECTS_H_

#include "libs.h"
#include "graphics/gl3/EffectMaterial.h"

namespace Graphics {
	class Renderer;
	namespace GL3 {
		class Effect;
	}
}

namespace Effects {

struct STerrainOptions
{
	STerrainOptions() : atmosphere(false), lava(false), water(false), eclipse(false) {}
	bool atmosphere;
	bool lava;
	bool water;
	bool eclipse;
};

class TerrainEffect : public Graphics::GL3::EffectMaterial
{
public:
	TerrainEffect(Graphics::Renderer* renderer, STerrainOptions& terrain_options);
	virtual ~TerrainEffect();

	virtual void Apply() override;

private:
	TerrainEffect(const TerrainEffect&);
	TerrainEffect& operator=(const TerrainEffect&);

	virtual void Init() override;

	STerrainOptions m_options;

	int m_atmosColorId;
	int m_geosphereScaleId;
	int m_geosphereScaledRadiusId;
	int m_geosphereAtmosTopRadId;
	int m_geosphereCenterId;
	int m_geosphereAtmosFogDensityId;
	int m_geosphereAtmosInvScaleHeightId;

	int m_shadowsId;
	int m_occultedLightId;
	int m_shadowCentreXId;
	int m_shadowCentreYId;
	int m_shadowCentreZId;
	int m_lradId;
	int m_sdivlradId;
};

class SkyEffect : public Graphics::GL3::EffectMaterial
{
public:
	explicit SkyEffect(Graphics::Renderer* renderer, bool eclipse = false);
	virtual ~SkyEffect();

	virtual void Apply() override;

private:
	SkyEffect(const SkyEffect&);
	SkyEffect& operator=(const SkyEffect&);

	virtual void Init() override;

	bool m_eclipse;
	
	int m_atmosColorId;
	int m_geosphereScaleId;
	int m_geosphereScaledRadiusId;
	int m_geosphereAtmosTopRadId;
	int m_geosphereCenterId;
	int m_geosphereAtmosFogDensityId;
	int m_geosphereAtmosInvScaleHeightId;

	int m_shadowsId;
	int m_occultedLightId;
	int m_shadowCentreXId;
	int m_shadowCentreYId;
	int m_shadowCentreZId;
	int m_lradId;
	int m_sdivlradId;
};

}


#endif
