// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "ShipCockpit.h"
#include "ShipType.h"
#include "Pi.h"
#include "WorldView.h"
#include "Player.h"
#include "Easing.h"
#include "graphics/TextureBuilder.h"
#include "graphics/effects/transit/TransitEffectMaterial.h"
#include "graphics/effects/transit/TransitCompositionMaterial.h"
#include "graphics/gl2/TexturedFullscreenQuad.h"
#include "graphics/PostProcessing.h"
#include "graphics/gl3/Effect.h"
#include "graphics/gl3/EffectMaterial.h"
#include "graphics/gl3/BuiltInShaders.h"

//--------------------------------------------------- CockpitTransitEffect
CockpitTransitEffect::CockpitTransitEffect(Graphics::Renderer* renderer) :
	m_renderer(renderer), m_time(0.0f)
{
	const int w = m_renderer->GetWindow()->GetWidth();
	const int h = m_renderer->GetWindow()->GetHeight();
	Graphics::MaterialDescriptor ttd;

	// Tunnel effect
	ttd.effect = Graphics::EffectType::EFFECT_TRANSIT_TUNNEL;
	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor desc;
		desc.uniforms.push_back("texture0");
		desc.uniforms.push_back("fViewportDimensions");
		desc.uniforms.push_back("fTime0_X");
		desc.uniforms.push_back("u_intensity");
		desc.uniforms.push_back("u_speed");
		desc.vertex_shader = "gl3/transit/tunnel.vert";
		desc.fragment_shader = "gl3/transit/tunnel.frag";
		m_tunnelMtrl.reset(new Graphics::GL3::EffectMaterial(m_renderer, desc));

		m_fViewportDimensionsId = m_tunnelMtrl->GetEffect()->GetUniformID("fViewportDimensions");
		m_fTime0_XId = m_tunnelMtrl->GetEffect()->GetUniformID("fTime0_X");
		m_intensityId = m_tunnelMtrl->GetEffect()->GetUniformID("u_intensity");
		m_speedId = m_tunnelMtrl->GetEffect()->GetUniformID("u_speed");
	} else {
		m_tunnelMtrl.reset(m_renderer->CreateMaterial(ttd));
	}

	// Radial blur effect
	ttd.effect = Graphics::EffectType::EFFECT_TRANSIT_COMPOSITION;
	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor desc;
		desc.uniforms.push_back("texture0");
		desc.uniforms.push_back("texture1");
		desc.uniforms.push_back("u_sampleDist");
		desc.uniforms.push_back("u_sampleStrength");
		desc.vertex_shader = "gl3/transit/composition.vert";
		desc.fragment_shader = "gl3/transit/composition.frag";
		m_compBlurMtrl.reset(new Graphics::GL3::EffectMaterial(m_renderer, desc));

		m_sampleDistId = m_compBlurMtrl->GetEffect()->GetUniformID("u_sampleDist");
		m_sampleStrengthId = m_compBlurMtrl->GetEffect()->GetUniformID("u_sampleStrength");
	} else {
		m_compBlurMtrl.reset(m_renderer->CreateMaterial(ttd));
	}

	// Fullscreen texture (for outputing)
	ttd.effect = Graphics::EffectType::EFFECT_TEXTURED_FULLSCREEN_QUAD;
	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptorDirect desc;
		desc.uniforms.push_back("texture0");
		desc.vertex_shader = Graphics::GL3::Shaders::FullscreenTexturedQuadVS;
		desc.vertex_shader_debug_name = "FullscreenVS";
		desc.fragment_shader = Graphics::GL3::Shaders::FullscreenTexturedQuadFS;
		desc.fragment_shader_debug_name = "FullscreenFS";
		m_outputMtrl.reset(new Graphics::GL3::EffectMaterial(m_renderer, desc));
	} else {
		m_outputMtrl.reset(
			reinterpret_cast<Graphics::GL2::TexturedFullscreenQuad*>(m_renderer->CreateMaterial(ttd)));
	}

	// Noise texture
	m_noiseTexture = 
		Graphics::TextureBuilder::UI("textures/noise.png").GetOrCreateTexture(m_renderer, "effect");

	// Tunnel render target
	Graphics::RenderTargetDesc rtd1( 
		std::max(2, w / 4), std::max(2, h / 4),
		Graphics::TextureFormat::TEXTURE_RGBA_8888, Graphics::TextureFormat::TEXTURE_NONE, false);
	m_tunnelRT.reset(m_renderer->CreateRenderTarget(rtd1));

	// Composite + radial blur render target
	Graphics::RenderTargetDesc rtd2(
		w, h,
		Graphics::TextureFormat::TEXTURE_RGBA_8888, Graphics::TextureFormat::TEXTURE_NONE, false);
	m_compBlurRT.reset(m_renderer->CreateRenderTarget(rtd2));
}

CockpitTransitEffect::~CockpitTransitEffect()
{

}

void CockpitTransitEffect::Update(float timeStep)
{
	const int w = m_renderer->GetWindow()->GetWidth();
	const int h = m_renderer->GetWindow()->GetHeight();
	
	float tunnel_intensity = 0.0; // 0.0 -> 0.35 -> 1.0
	float tunnel_speed = 0.0; // 0.25 -> 0.5 -> 1.456789
	float rblur_dist = 0.0;	// 0.0 -> 1.0 -> 2.0
	float rblur_str = 0.0; // 0.0 -> 2.0 -> 3.5	
	float ship_speed = Pi::player->GetVelocity().Length();
	if(ship_speed <= TRANSIT_DRIVE_1_SPEED) {
		float p = ship_speed / TRANSIT_DRIVE_1_SPEED;
		tunnel_intensity = p * 0.35f;
		tunnel_speed = 0.25f + (p * 0.25f);
		rblur_dist = p * 1.0f;
		rblur_str = p * 2.0f;
	} else {
		float p = ship_speed / TRANSIT_DRIVE_2_SPEED;
		tunnel_intensity = 0.35f + (p * 0.65f);
		tunnel_speed = 0.5f + (p * 0.956789f);
		rblur_dist = 1.0f + p;
		rblur_str = 2.0f + (p * 1.5f);
	}

	if(Graphics::Hardware::GL3()) {
		m_tunnelMtrl->GetEffect()->SetProgram();
		m_tunnelMtrl->GetEffect()->GetUniform(m_fViewportDimensionsId).Set(
			vector2f(std::max(2, w/4), std::max(2, h/4)));
		m_time += timeStep;
		if (m_time > 25.0f) {
			m_time = 5.0f;
		}
		m_tunnelMtrl->GetEffect()->GetUniform(m_fTime0_XId).Set(m_time);
		m_tunnelMtrl->GetEffect()->GetUniform(m_intensityId).Set(tunnel_intensity * 0.5f);
		m_tunnelMtrl->GetEffect()->GetUniform(m_speedId).Set(tunnel_speed);

		m_compBlurMtrl->GetEffect()->SetProgram();
		m_compBlurMtrl->GetEffect()->GetUniform(m_sampleDistId).Set(rblur_dist);
		m_compBlurMtrl->GetEffect()->GetUniform(m_sampleStrengthId).Set(rblur_str);
	} else {
		auto tm = reinterpret_cast<Graphics::Effects::TransitEffectMaterial*>(m_tunnelMtrl.get());
		tm->UpdateParams(
			std::max(2, w / 4), std::max(2, h / 4),
			timeStep);
		tm->SetTunnelParams(tunnel_intensity * 0.5f, tunnel_speed);
		auto cm = reinterpret_cast<Graphics::Effects::TransitCompositionMaterial*>(m_compBlurMtrl.get());
		cm->SetRadialBlur(rblur_dist, rblur_str);
	}
}

void CockpitTransitEffect::Render(const matrix4x4d &viewTransform)
{
	Graphics::RenderTarget* current_rt = m_renderer->GetActiveRenderTarget();
	assert(current_rt);
	if(!current_rt) {
		//Error("Current RT is NULL! bPerformPostProcessing: %d", 
		//	Pi::renderer->GetPostProcessing()->IsPostProcessingEnabled());
		return;
	}

	static Graphics::RenderState* rs = nullptr;
	if (!rs) {
		Graphics::RenderStateDesc rsd;
		rsd.blendMode = Graphics::BlendMode::BLEND_SOLID;
		rsd.depthTest = false;
		rsd.depthWrite = false;
		rs = m_renderer->CreateRenderState(rsd);
	}
	
	// Render tunnel effect
	m_renderer->SetRenderTarget(m_tunnelRT.get());
	m_tunnelMtrl->texture0 = m_noiseTexture;
	m_renderer->DrawFullscreenQuad(m_tunnelMtrl.get(), rs, false);

	// Combine both with a transit radial blur effect
	m_renderer->SetRenderTarget(m_compBlurRT.get());
	m_compBlurMtrl->texture0 = current_rt->GetColorTexture();
	m_compBlurMtrl->texture1 = m_tunnelRT->GetColorTexture();
	m_renderer->DrawFullscreenQuad(m_compBlurMtrl.get(), rs, false);

	// Draw result to output RT
	m_renderer->SetRenderTarget(current_rt);
	m_outputMtrl->texture0 = m_compBlurRT->GetColorTexture();
	m_renderer->DrawFullscreenQuad(m_outputMtrl.get(), rs, false);
}


//--------------------------------------------------- ShipCockpit
ShipCockpit::ShipCockpit(const std::string &modelName) :
	m_shipDir(0.0),
	m_shipYaw(0.0),
	m_dir(0.0),
	m_yaw(0.0),
	m_rotInterp(0.f),
	m_transInterp(0.f),
	m_gForce(0.f),
	m_offset(0.f),
	m_shipVel(0.f),
	m_translate(0.0),
	m_transform(matrix4x4d::Identity())
{
	assert(!modelName.empty());
	SetModel(modelName.c_str());
	assert(GetModel());
	SetColliding(false);
	m_transitEffect.reset(new CockpitTransitEffect(Pi::renderer));
}

ShipCockpit::~ShipCockpit()
{

}

void ShipCockpit::Update(float timeStep)
{
	m_transform = matrix4x4d::Identity();
	m_translate = vector3d(0.0, 0.0, 0.0);
	m_rotInterp = 0.0f;
	m_transInterp = 0.0f;

	if (Pi::IsPostProcessingEnabled() && 
		Pi::player->GetTransitState() == TransitState::TRANSIT_DRIVE_ON) 
	{
		m_transitEffect->Update(timeStep);
	}

	vector3d cur_dir = Pi::player->GetOrient().VectorZ().Normalized();
	if(cur_dir.Dot(m_shipDir) < 1.0f) {
		m_rotInterp = 0.0f;
		m_shipDir = cur_dir;
	}

	//---------------------------------------- Acceleration
	float cur_vel = CalculateSignedForwardVelocity(-cur_dir, Pi::player->GetVelocity()); // Forward is -Z
	float gforce = Clamp(floorf(((abs(cur_vel) - m_shipVel) / timeStep) / 9.8f), -COCKPIT_MAX_GFORCE, COCKPIT_MAX_GFORCE);
	if(abs(cur_vel) > 500000.0f ||      // Limit gforce measurement so we don't get astronomical fluctuations
	   abs(gforce - m_gForce) > 100.0) { // Smooth out gforce on frame spikes, sometimes happens when hitting max speed due to the thrust limiters
		gforce = 0.0f;
	}
	if((gforce > 0 && m_gForce < 0) || (m_gForce > 0 && gforce < 0)) {
		gforce = 0.0f;
	}
	if(abs(m_translate.z - m_offset) < 0.001f) {
		m_transInterp = 0.0f;
	}
	float offset = (gforce > 14.0f? -1.0f : (gforce < -14.0f? 1.0f : 0.0f)) * COCKPIT_ACCEL_OFFSET;
	m_transInterp += timeStep * COCKPIT_ACCEL_INTERP_MULTIPLIER;
	if(m_transInterp > 1.0f) {
		m_transInterp = 1.0f;	
		m_translate.z = offset;
	}
	m_translate.z = Easing::Quad::EaseIn(double(m_transInterp), m_translate.z, offset-m_translate.z, 1.0);
	m_gForce = gforce;
	m_offset = offset;
	m_shipVel = cur_vel;

	//------------------------------------------ Rotation
	// For yaw/pitch
	vector3d rot_axis = cur_dir.Cross(m_dir).Normalized();
	vector3d yaw_axis = Pi::player->GetOrient().VectorY().Normalized();
	vector3d pitch_axis = Pi::player->GetOrient().VectorX().Normalized();
	float dot = cur_dir.Dot(m_dir);
	float angle = acos(dot);
	// For roll
	if(yaw_axis.Dot(m_shipYaw) < 1.0f) {
		m_rotInterp = 0.0f;
		m_shipYaw = yaw_axis;
	}
	vector3d rot_yaw_axis = yaw_axis.Cross(m_yaw).Normalized();
	float dot_yaw = yaw_axis.Dot(m_yaw);
	float angle_yaw = acos(dot_yaw);

	if(dot < 1.0f || dot_yaw < 1.0f) {
		// Lag/Recovery interpolation
		m_rotInterp += timeStep * COCKPIT_ROTATION_INTERP_MULTIPLIER;
		if(m_rotInterp > 1.0f) {
			m_rotInterp = 1.0f;
		}

		// Yaw and Pitch
		if(dot < 1.0f) {
			if(angle > DEG2RAD(COCKPIT_LAG_MAX_ANGLE)) {
				angle = DEG2RAD(COCKPIT_LAG_MAX_ANGLE);
			}
			angle = Easing::Quad::EaseOut(m_rotInterp, angle, -angle, 1.0f);
			m_dir = cur_dir;
			if(angle >= 0.0f) {
				m_dir.ArbRotate(rot_axis, angle);
				// Apply pitch
				vector3d yz_proj = (m_dir - (m_dir.Dot(pitch_axis) * pitch_axis)).Normalized();
				float pitch_cos = yz_proj.Dot(cur_dir);
				float pitch_angle = 0.0f;
				if(pitch_cos < 1.0f) {
					pitch_angle = acos(pitch_cos);
					if(rot_axis.Dot(pitch_axis) < 0) {
						pitch_angle = -pitch_angle;
					}
					m_transform.RotateX(-pitch_angle);
				}
				// Apply yaw
				vector3d xz_proj = (m_dir - (m_dir.Dot(yaw_axis) * yaw_axis)).Normalized();
				float yaw_cos = xz_proj.Dot(cur_dir);
				float yaw_angle = 0.0f;
				if(yaw_cos < 1.0f) {
					yaw_angle = acos(yaw_cos);
					if(rot_axis.Dot(yaw_axis) < 0) {
						yaw_angle = -yaw_angle;
					}
					m_transform.RotateY(-yaw_angle);
				}
			} else {
				angle = 0.0f;
			}
		}

		// Roll
		if(dot_yaw < 1.0f) {			
			if(angle_yaw > DEG2RAD(COCKPIT_LAG_MAX_ANGLE)) {
				angle_yaw = DEG2RAD(COCKPIT_LAG_MAX_ANGLE);
			}
			if(dot_yaw < 1.0f) {
				angle_yaw = Easing::Quad::EaseOut(m_rotInterp, angle_yaw, -angle_yaw, 1.0f);
			}
			m_yaw = yaw_axis;
			if(angle_yaw >= 0.0f) {
				m_yaw.ArbRotate(rot_yaw_axis, angle_yaw);
				// Apply roll
				vector3d xy_proj = (m_yaw - (m_yaw.Dot(cur_dir) * cur_dir)).Normalized();
				float roll_cos = xy_proj.Dot(yaw_axis);
				float roll_angle = 0.0f;
				if(roll_cos < 1.0f) {
					roll_angle = acos(roll_cos);
					if(rot_yaw_axis.Dot(cur_dir) < 0) {
						roll_angle = -roll_angle;
					}
					m_transform.RotateZ(-roll_angle);
				}
			} else {
				angle_yaw = 0.0f;
			}
		}
	} else {
		m_rotInterp = 0.0f;
	}

	// Freelook transformation
	if(Pi::GetView() && Pi::GetView() == dynamic_cast<View*>(Pi::worldView)) {
		if (Pi::worldView->GetCameraController()->GetType() == CameraController::Type::INTERNAL) {
			InternalCameraController* cam = static_cast<InternalCameraController*>(
				Pi::worldView->GetCameraController());
			if(cam->IsFreelooking()) {
				m_transform = m_transform * matrix4x4d(cam->GetExtOrient().Transpose());
			}
		}
	}
}

void ShipCockpit::RenderCockpit(Graphics::Renderer* renderer, const Camera* camera, Frame* frame)
{
	renderer->ClearDepthBuffer();
	SetFrame(frame);
	
	if(Pi::IsPostProcessingEnabled() &&
		Pi::player->GetTransitState() == TransitState::TRANSIT_DRIVE_ON &&
		Pi::player->GetVelocity().Length() > 20000.0) 
	{
		m_transitEffect->Render(m_transform);
	}
	Render(renderer, camera, m_translate, m_transform);
	SetFrame(nullptr);
}

void ShipCockpit::Render(Graphics::Renderer *renderer, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform)
{
	if(Pi::player) {
		GetModel()->CalcAtmosphericProperties(this, Pi::player->GetFrame());
	} else {
		GetModel()->SetAtmosphericProperties(0.0f, Color(0));
	}
	RenderModel(renderer, camera, viewCoords, viewTransform);
}

void ShipCockpit::OnActivated()
{
	assert(Pi::player);
	m_dir = Pi::player->GetOrient().VectorZ().Normalized();
	m_yaw = Pi::player->GetOrient().VectorY().Normalized();
	m_shipDir = m_dir;
	m_shipYaw = m_yaw;
	m_shipVel = CalculateSignedForwardVelocity(-m_shipDir, Pi::player->GetVelocity());
}

void ShipCockpit::Shake(double sx, double sy)
{
	m_translate.x = sx;
	m_translate.y = sy;
}

float ShipCockpit::CalculateSignedForwardVelocity(vector3d normalized_forward, vector3d velocity) {
	float velz_cos = velocity.Dot(normalized_forward);
	return (velz_cos * normalized_forward).Length() * (velz_cos < 0.0? -1.0 : 1.0);
}
