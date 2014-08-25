// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_TRANSIT_EFFECT_MATERIAL_H_
#define _GL2_TRANSIT_EFFECT_MATERIAL_H_

/*
* Draws sector view icons.
*/
#include "libs.h"
#include "../../gl2/GL2Material.h"
#include "../../gl2/Program.h"
#include "../../RendererGL2.h"

namespace Graphics {
	namespace Effects {
		class TransitEffectMaterial : public GL2::Material {
		public:
			TransitEffectMaterial() {
				texture0 = nullptr;
				texture1 = nullptr;
				texture2 = nullptr;
				m_time = 0.0f;
				m_tunnelIntensity = 1.0f;
				m_tunnelSpeed = 1.0f;
			}

			GL2::Program *CreateProgram(const MaterialDescriptor &) {
				return new GL2::Program("transit/t_tunnel", "");
			}

			virtual void SetProgram(GL2::Program *p) override {
				GL2::Material::SetProgram(p);
				m_program->AddUniform(m_windowSizeUniform, "fViewportDimensions");
				m_program->AddUniform(m_timeUniform, "fTime0_X");
				m_program->AddUniform(m_tunnelIntensityUniform, "u_intensity");
				m_program->AddUniform(m_tunnelSpeedUniform, "u_speed");
			}

			virtual void Apply() {
				m_program->Use();
				m_program->invLogZfarPlus1.Set(m_renderer->m_invLogZfarPlus1);
				if(texture0) {
					m_program->texture0.Set(texture0, 0);
				}
				if(texture1) {
					m_program->texture1.Set(texture1, 1);
				}
				if(texture2) {
					m_program->texture2.Set(texture2, 2);
				}
				m_windowSizeUniform.Set(m_windowSize);
				m_timeUniform.Set(m_time);
				m_tunnelIntensityUniform.Set(m_tunnelIntensity);
				m_tunnelSpeedUniform.Set(m_tunnelSpeed);
			}

			virtual void Unapply() {
				m_program->Unuse();
			}

			void UpdateParams(unsigned int width, unsigned int height, float timeStep) {
				m_windowSize.x = width;
				m_windowSize.y = height;
				m_time += timeStep;
				if(m_time > 25.0f) {
					m_time = 5.0f;
				}
			}

			void SetTunnelParams(float intensity, float speed) {
				m_tunnelIntensity = intensity;
				m_tunnelSpeed = speed;
			}

		private:
			GL2::Uniform m_windowSizeUniform;
			GL2::Uniform m_timeUniform;
			GL2::Uniform m_tunnelIntensityUniform;
			GL2::Uniform m_tunnelSpeedUniform;
			vector2f m_windowSize;
			float m_time;
			float m_tunnelIntensity;
			float m_tunnelSpeed;
		};
	}
}

#endif