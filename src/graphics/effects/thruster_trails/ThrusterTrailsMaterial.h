// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_THRUSTER_TRAILS_MATERIAL_H_
#define _GL2_THRUSTER_TRAILS_MATERIAL_H_

/*
 * Performs a gaussian horizontal blur on a fullscreen quad.
 */
#include "libs.h"
#include "../../gl2/GL2Material.h"
#include "../../gl2/Program.h"
#include "../../RendererGL2.h"

namespace Graphics {
	namespace Effects {
		class ThrusterTrailsMaterial : public GL2::Material {
		public:
			ThrusterTrailsMaterial() {
				texture0 = nullptr;
				texture1 = nullptr;
			}

			GL2::Program *CreateProgram(const MaterialDescriptor &) {
				return new GL2::Program("thruster_trails/tt_draw", "");
			}

			virtual void SetProgram(GL2::Program *p) override {
				GL2::Material::SetProgram(p);
				m_program->AddUniform(m_windowSizeUniform, "u_windowSize");
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
				m_windowSizeUniform.Set(m_windowSize);
			}

			virtual void Unapply() {
				m_program->Unuse();
			}

			void setWindowSize(unsigned int width, unsigned int height) {
				m_windowSize.x = width;
				m_windowSize.y = height;
				m_windowSize.z = 0.0f;
			}

		private:
			GL2::Uniform m_windowSizeUniform;
			vector3f m_windowSize;
		};
	}
}

#endif