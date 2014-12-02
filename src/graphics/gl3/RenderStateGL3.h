// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _RENDERSTATE_GL3_H
#define _RENDERSTATE_GL3_H
#include "graphics/RenderState.h"

namespace Graphics { namespace GL3 {

class RenderState : public Graphics::RenderState {
public:
	RenderState(const RenderStateDesc&);
	void Apply();
};

}}
#endif
