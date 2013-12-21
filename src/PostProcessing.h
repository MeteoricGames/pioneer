// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _POST_PROCESSING_H_
#define _POST_PROCESSING_H_

#include "libs.h"
#include "graphics/Material.h"

namespace Graphics {
	namespace GL2 {
		class PostProcess;

		class PostProcessing
		{
		public:
			PostProcessing();
			virtual ~PostProcessing();
	
			void Run(PostProcess* pp = nullptr);

		protected:

		private:
			PostProcessing(const PostProcessing&);
			PostProcessing& operator=(const PostProcessing&);

			void Init();
		
			std::unique_ptr<Material> mtrlFullscreenQuad;
			unsigned int uScreenQuadBufferId;
			RenderTarget* rtMain;
		};
	}
}

#endif
