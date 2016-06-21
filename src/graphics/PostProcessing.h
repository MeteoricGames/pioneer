// Copyright � 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _POST_PROCESSING_H_
#define _POST_PROCESSING_H_

#include "libs.h"
#include "PostProcessLayer.h"

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
	
		void BeginFrame(PostProcessLayer layer = EPP_LAYER_GAME);
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
		Renderer* m_renderer;
		RenderTarget* m_rtDevice;
		RenderState* m_renderState;
		RenderState* m_renderStateLayer; // Used for layering
		bool m_bPerformPostProcessing;
		RenderTarget* m_rtMain;
		RenderTarget* m_rtTemp;
		PostProcessLayer m_currentLayer;
	};
}

#endif
