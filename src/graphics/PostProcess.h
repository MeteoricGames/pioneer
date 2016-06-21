// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _POST_PROCESS_H_
#define _POST_PROCESS_H_

#include "libs.h"
#include "RenderTarget.h"
#include "Material.h"

namespace Graphics {
	class WindowSDL;
	class Material;
	class Renderer;
	class PostProcessing;

	namespace GL3 {
		class Effect;
	}

	enum PostProcessPassType
	{
		PP_PASS_THROUGH, // Normal pass: last pass as input, new pass as output
		PP_PASS_COMPOSE, // Composition pass: main rt and last pass as input, new pass as output
	};

	enum PostProcessEffectType
	{
		PP_ET_MATERIAL = 0,
		PP_ET_EFFECT,
	};

	struct PostProcessPass
	{
		PostProcessPass() : effect_type(PP_ET_MATERIAL), texture0Id(-1), texture1Id(-1), bypass(false) {}
		std::string name;
		std::shared_ptr<Material> material;
		std::shared_ptr<Graphics::GL3::Effect> effect;
		std::unique_ptr<RenderTarget> renderTarget;
		PostProcessPassType type;
		PostProcessEffectType effect_type;
		int texture0Id;
		int texture1Id;
		// Bypass deactivates this pass but only when it's not the first or last pass in the process.
		bool bypass;
	};

	class PostProcess
	{
		friend class PostProcessing;
	public:
		// Constructs a custom render target
		PostProcess(const std::string& effect_name, RenderTargetDesc& rtd);
		// Constructs a color render target that matches the display mode
		PostProcess(const std::string& effect_name, WindowSDL* window, bool with_alpha = false);
		virtual ~PostProcess();

		unsigned AddPass(Renderer* renderer, const std::string& pass_name, 
			std::shared_ptr<Material>& material, PostProcessPassType pass_type = PP_PASS_THROUGH);
		unsigned AddPass(Renderer* renderer, const std::string& pass_name, 
			Graphics::EffectType effect_type, PostProcessPassType pass_type = PP_PASS_THROUGH);
		unsigned AddPass(Renderer* renderer, const std::string& pass_name,
			std::shared_ptr<Graphics::GL3::Effect> effect, 
			PostProcessPassType pass_type = PP_PASS_THROUGH);
		bool GetBypassState(unsigned pass_id) const;
		void SetBypassState(unsigned pass_id, bool bypass);

		// Accessors
		unsigned int GetPassCount() const { return vPasses.size(); }

	protected:		

	private:
		PostProcess(const PostProcess&);
		PostProcess& operator=(const PostProcess&);

		std::string strName;
		std::vector<PostProcessPass*> vPasses;
		std::unique_ptr<RenderTargetDesc> mRTDesc;
	};
}

#endif
