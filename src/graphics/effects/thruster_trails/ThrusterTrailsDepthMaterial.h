// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_THRUSTER_TRAILS_DEPTH_MATERIAL_H_
#define _GL2_THRUSTER_TRAILS_DEPTH_MATERIAL_H_

/*
 * Draws trails to illustrate depth.
 * Required render state:
 * - BlendDestAlpha ONE
 * - BlendDestRGB ONE
 * - BlendEnable TRUE
 * - BlendSourceAlpha ONE
 * - BlendSourceRGB ONE
 * - DepthEnable FALSE
 */
#include "libs.h"
#include "../../gl2/GL2Material.h"
#include "../../gl2/Program.h"

namespace Graphics {
	namespace Effects {
		class ThrusterTrailsDepthMaterial : public GL2::Material {
		public:
			ThrusterTrailsDepthMaterial() {
			}

			GL2::Program *CreateProgram(const MaterialDescriptor &) {
				return new GL2::Program("thruster_trails/tt_depth", "");
			}

			virtual void Apply() {
				m_program->Use();
				m_program->invLogZfarPlus1.Set(m_renderer->m_invLogZfarPlus1);
			}

			virtual void Unapply() {
				m_program->Unuse();
			}
		};
	}
}

#endif