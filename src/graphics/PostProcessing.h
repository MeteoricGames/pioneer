// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _POST_PROCESSING_H_
#define _POST_PROCESSING_H_

#include "libs.h"

namespace Graphics {
	class Renderer;
	class PostProcess;
	class RenderTarget;
	class RenderState;

	namespace GL3 { class EffectMaterial; }

	class PostProcessing
	{
	public:
		explicit PostProcessing(Renderer *renderer);
		virtual ~PostProcessing();
	
		void BeginFrame();
		void EndFrame();
		void Run(PostProcess* pp = nullptr);
		void SetPerformPostProcessing(bool enabled);
		void SetDeviceRT(RenderTarget* rt_device);
		bool IsPostProcessingEnabled() const { return m_bPerformPostProcessing; }

	protected:

	private:
		PostProcessing(const PostProcessing&);
		PostProcessing& operator=(const PostProcessing&);

		void Init();
		
		std::unique_ptr<GL3::EffectMaterial> m_mtrlFullscreenQuad;
		int m_uScreenQuadBufferId;		
		Renderer* m_renderer;
		RenderTarget* m_rtDevice;
		RenderTarget* m_rtMain;
		RenderState* m_renderState;
		bool m_bPerformPostProcessing;
	};
}

#endif
