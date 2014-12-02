// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SMAA_H_
#define _SMAA_H_

#include "libs.h"

namespace Graphics { 
	class Renderer; 
	class PostProcess; 
	class Texture;
	namespace GL3 {
		class Effect;
	}
}

class SMAA
{
public:
	SMAA(Graphics::Renderer* renderer);
	virtual ~SMAA();

	void PostProcess();

private:
	SMAA(const SMAA&);
	SMAA& operator=(const SMAA&);

	void Init();

	Graphics::Renderer* m_renderer;
	std::unique_ptr<Graphics::PostProcess> m_postprocess;
	std::shared_ptr<Graphics::GL3::Effect> m_pass1; // Luma Edge Detection
	std::shared_ptr<Graphics::GL3::Effect> m_pass2; // Blending Weight Calculation
	std::shared_ptr<Graphics::GL3::Effect> m_pass3; // Neighbourhood Blending

	Graphics::Texture* m_areaTexture;
	Graphics::Texture* m_searchTexture;
	unsigned int m_areaTexUniformID;
	unsigned int m_searchTexUniformID;
	unsigned int m_p3ColorTexUniformID;
};

#endif