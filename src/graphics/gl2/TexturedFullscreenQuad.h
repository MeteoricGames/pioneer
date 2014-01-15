// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_TEXTURED_FULLSCREEN_QUAD_H_
#define _GL2_TEXTURED_FULLSCREEN_QUAD_H_

/*
 * Textured fullscreen quad.
 * A simple shader to draw a fullscreen textured quad, useful for multipass effects.
 */
#include "libs.h"
#include "Program.h"

namespace Graphics {
	namespace GL2 {
		class TexturedFullscreenQuad : public Material {
		public:
			TexturedFullscreenQuad() {
				texture0 = nullptr;
			}

			Program *CreateProgram(const MaterialDescriptor &) {
				return new Program("pp_textured_fullscreen_quad", "");
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