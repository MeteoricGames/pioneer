// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_BLOOM_COMPOSITOR_MATERIAL_H_
#define _GL2_BLOOM_COMPOSITOR_MATERIAL_H_

/*
 * Combines scene with gaussian blurred copy to perform bloom effect.
 */
#include "libs.h"
#include "Program.h"

namespace Graphics {
	namespace GL2 {
		class BloomCompositorMaterial : public Material {
		public:
			BloomCompositorMaterial() {
				texture0 = nullptr;
				texture1 = nullptr;
			}

			Program *CreateProgram(const MaterialDescriptor &) {
				return new Program("pp_compositor", "");
			}

			virtual void Apply() {
				m_program->Use();
				if(texture0) {
					m_program->texture0.Set(texture0, 0);
				}
				if(texture1) {
					m_program->texture1.Set(texture1, 1);
				}
			}

			virtual void Unapply() {
				m_program->Unuse();
			}
		};
	}
}

#endif
