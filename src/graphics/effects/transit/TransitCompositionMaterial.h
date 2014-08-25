// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GL2_TRANSIT_COMPOSITION_MATERIAL_H_
#define _GL2_TRANSIT_COMPOSITION_MATERIAL_H_

/*
* Draws sector view icons.
*/
#include "../RadialBlurMaterial.h"

namespace Graphics {
	namespace Effects {
		class TransitCompositionMaterial : public RadialBlurMaterial {
		public:
			TransitCompositionMaterial() {
			}

			virtual GL2::Program *CreateProgram(const MaterialDescriptor &) override {
				return new GL2::Program("transit/t_composition", "");
			}

		};
	}
}

#endif