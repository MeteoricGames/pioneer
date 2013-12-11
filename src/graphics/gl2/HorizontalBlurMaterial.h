#ifndef _GL2_HORIZONTAL_BLUR_MATERIAL_H_
#define _GL2_HORIZONTAL_BLUR_MATERIAL_H_

/*
 * Performs a gaussian horizontal blur on a fullscreen quad.
 */
#include "libs.h"
#include "Program.h"

namespace Graphics {
	namespace GL2 {
		class HorizontalBlurMaterial : public Material {
		public:
			HorizontalBlurMaterial() {
				texture0 = nullptr;
			}

			Program *CreateProgram(const MaterialDescriptor &) {
				return new Program("pp_hblur", "");
			}

			virtual void Apply() {
				m_program->Use();
				if(texture0) {
					m_program->texture0.Set(texture0, 0);
				}
			}

			virtual void Unapply() {
				m_program->Unuse();
			}
		};
	}
}

#endif