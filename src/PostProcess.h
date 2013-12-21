// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _POST_PROCESS_H_
#define _POST_PROCESS_H_

#include "libs.h"
#include "graphics/Material.h"
#include "graphics/RenderTarget.h"

namespace Graphics {
	namespace GL2 {
		struct PostProcessPass
		{
			std::string name;
			std::shared_ptr<Material> material;
			std::unique_ptr<RenderTarget> renderTarget;
		};

		class PostProcess
		{
		public:
			PostProcess(const std::string& effect_name, RenderTargetDesc& rtd);
			explicit PostProcess(const std::string& effect_name);
			virtual ~PostProcess();

			void AddPass(const std::string& pass_name, std::shared_ptr<Material>& material);

		protected:

		private:
			PostProcess(const PostProcess&);
			PostProcess& operator=(const PostProcess&);

			std::string strName;
			std::vector<PostProcessPass*> vPasses;
			std::unique_ptr<RenderTargetDesc> mRTDesc;
		};
	}
}

#endif