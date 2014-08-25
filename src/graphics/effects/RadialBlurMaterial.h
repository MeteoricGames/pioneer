// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_RADIAL_BLUR_MATERIAL_H_
#define _GL2_RADIAL_BLUR_MATERIAL_H_

/*
* Draws sector view icons.
*/
#include "libs.h"
#include "../gl2/GL2Material.h"
#include "../gl2/Program.h"
#include "../RendererGL2.h"

namespace Graphics {
	namespace Effects {
		class RadialBlurMaterial : public GL2::Material {
		public:
			RadialBlurMaterial() {
				texture0 = nullptr;
				texture1 = nullptr;
				texture2 = nullptr;
				m_sampleDist = 2.3f;
				m_sampleStrength = 4.2f;
			}

			virtual GL2::Program *CreateProgram(const MaterialDescriptor &) override {
				return new GL2::Program("pp_radialblur", "");
			}

			virtual void SetProgram(GL2::Program *p) override {
				GL2::Material::SetProgram(p);
				m_program->AddUniform(m_sampleDistUniform, "u_sampleDist");
				m_program->AddUniform(m_sampleStrengthUniform, "u_sampleStrength");
			}

			virtual void Apply() {
				m_program->Use();
				m_program->invLogZfarPlus1.Set(m_renderer->m_invLogZfarPlus1);
				if(texture0) {
					m_program->texture0.Set(texture0, 0);
				}
				if (texture1) {
					m_program->texture1.Set(texture1, 1);
				}
				if (texture2) {
					m_program->texture2.Set(texture2, 2);
				}
				m_sampleDistUniform.Set(m_sampleDist);
				m_sampleStrengthUniform.Set(m_sampleStrength);
			}

			virtual void Unapply() {
				m_program->Unuse();
			}

			void SetRadialBlur(float dist, float strength) {
				m_sampleDist = dist;
				m_sampleStrength = strength;
			}

		protected:
			GL2::Uniform m_sampleDistUniform;
			GL2::Uniform m_sampleStrengthUniform;
			float m_sampleDist;
			float m_sampleStrength;
		};
	}
}

#endif