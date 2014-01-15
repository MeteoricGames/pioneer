// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_SKYBOX_MATERIAL_H_
#define _GL2_SKYBOX_MATERIAL_H_

/*
 * Renders a cube map as a skybox.
 */
#include "libs.h"
#include "Program.h"

namespace Graphics {
	namespace GL2 {
		class SkyboxMaterial : public Material {
		private:

		public:
			SkyboxMaterial() {
				texture0 = nullptr;
				specialParameter0 = nullptr;
				fSkyboxFactor = 0.8f;
				fIntensity = 1.0f;
			}

			Program *CreateProgram(const MaterialDescriptor &) {
				return new Program("skybox", "");
			}

			virtual void Apply() {
				m_program->Use();
				if(texture0) {
					m_program->texture0.Set(texture0, 0);
				}
				if(specialParameter0) {
					fIntensity = *(static_cast<float*>(specialParameter0));
				}
				m_program->shininess.Set(fSkyboxFactor * fIntensity);
				glPushAttrib(GL_DEPTH_BUFFER_BIT);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
			}

			virtual void Unapply() {
				glPopAttrib();
				m_program->Unuse();
			}
			
			float fSkyboxFactor;
			float fIntensity;
		};
	}
}

#endif
