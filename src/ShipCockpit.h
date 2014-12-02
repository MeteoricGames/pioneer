// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SHIP_COCKPIT_H_
#define _SHIP_COCKPIT_H_

#include "libs.h"
#include "ModelBody.h"
#include "scenegraph/Model.h"

static const float COCKPIT_LAG_MAX_ANGLE = 7.5f;
static const float COCKPIT_ROTATION_INTERP_MULTIPLIER = 5.0f;
static const float COCKPIT_ACCEL_INTERP_MULTIPLIER = 0.5f;
static const float COCKPIT_MAX_GFORCE = 10000.0f;
static const float COCKPIT_ACCEL_OFFSET = 0.075f;
static const double COCKPIT_MAX_FREELOOK_ANGLE = 30.0;
static const double COCKPIT_MAX_FREELOOK_SPEED = 45.0;
static const float COCKPIT_RESET_TIME = 2.0f;

namespace Graphics { 
	class Material; 
	namespace GL3
	{
		class EffectMaterial;
	}
	namespace GL2 
	{ 
		class TexturedFullscreenQuad; 
	} 
	namespace Effects 
	{ 
		class TransitEffectMaterial; 
		class TransitCompositionMaterial; 
	}
}

class CockpitTransitEffect
{
public:
	explicit CockpitTransitEffect(Graphics::Renderer* renderer);
	~CockpitTransitEffect();

	void Update(float timeStep);
	void Render(const matrix4x4d &viewTransform);

private:
	CockpitTransitEffect(const CockpitTransitEffect&);
	CockpitTransitEffect& operator=(const CockpitTransitEffect&);

	Graphics::Renderer* m_renderer;
	std::unique_ptr<Graphics::Material> m_tunnelMtrl;
	std::unique_ptr<Graphics::Material> m_compBlurMtrl;
	std::unique_ptr<Graphics::Material> m_outputMtrl;
	std::unique_ptr<Graphics::RenderTarget> m_tunnelRT;
	std::unique_ptr<Graphics::RenderTarget> m_compBlurRT;
	Graphics::Texture* m_noiseTexture;
	float m_time;

	int m_fViewportDimensionsId;
	int m_fTime0_XId;
	int m_intensityId;
	int m_speedId;

	int m_sampleDistId;
	int m_sampleStrengthId;
};

class ShipCockpit : public ModelBody
{
public:
	explicit ShipCockpit(const std::string &modelName);
	virtual ~ShipCockpit();

	virtual void Render(Graphics::Renderer *r, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform) override;

	void Update(float timeStep);
	void RenderCockpit(Graphics::Renderer* renderer, const Camera* camera, Frame* frame);
	void OnActivated();
	void Shake(double sx, double sy);

protected:
	float CalculateSignedForwardVelocity(vector3d forward, vector3d velocity);

private:
	ShipCockpit(const ShipCockpit&);
	ShipCockpit& operator=(const ShipCockpit&);

	vector3d m_shipDir;        // current ship direction
	vector3d m_shipYaw;        // current ship yaw vector
	vector3d m_dir;            // cockpit direction
	vector3d m_yaw;            // cockpit yaw vector
	float m_rotInterp;         // for rotation interpolation
	float m_transInterp;       // for translation interpolation
	float m_gForce;            // current ship gforce
	float m_offset;            // current ship offset due to acceleration effect
	float m_shipVel;           // current ship velocity
	vector3d m_translate;      // cockpit translation
	matrix4x4d m_transform;    // cockpit transformation

	std::unique_ptr<CockpitTransitEffect> m_transitEffect;
};

#endif
