// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_SECTOR_VIEW_ICON_MATERIAL_H_
#define _GL2_SECTOR_VIEW_ICON_MATERIAL_H_

/*
* Draws sector view icons.
*/
#include "libs.h"
#include "../../gl2/GL2Material.h"
#include "../../gl2/Program.h"

namespace Graphics {
	namespace Effects {
		class SectorViewIconMaterial : public GL2::Material {
		public:
			SectorViewIconMaterial() {
				texture0 = nullptr;
			}

			GL2::Program *CreateProgram(const MaterialDescriptor &) {
				return new GL2::Program("sector_view/sv_icon", "");
			}

			virtual void SetProgram(GL2::Program *p) override {
				GL2::Material::SetProgram(p);
			}

			virtual void Apply() {
				m_program->Use();
				m_program->invLogZfarPlus1.Set(m_renderer->m_invLogZfarPlus1);
				if (texture0) {
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