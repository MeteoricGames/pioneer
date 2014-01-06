// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _POST_PROCESSING_H_
#define _POST_PROCESSING_H_

#include "libs.h"
#include "Material.h"

namespace Graphics {
	class Renderer;
	class PostProcess;

	class PostProcessing
	{
	public:
		explicit PostProcessing(Renderer *renderer);
		virtual ~PostProcessing();
	
		void BeginFrame();
		void EndFrame();
		void Run(PostProcess* pp = nullptr);
		void SetEnabled(bool enabled);
		void SetDeviceRT(RenderTarget* rt_device);

	protected:

	private:
		PostProcessing(const PostProcessing&);
		PostProcessing& operator=(const PostProcessing&);

		void Init();
		
		std::unique_ptr<Material> mtrlFullscreenQuad;
		unsigned int uScreenQuadBufferId;
		RenderTarget* rtDevice;
		RenderTarget* rtMain;
		Renderer* mRenderer;
		bool bPerformPostProcessing;
	};
}

#endif
