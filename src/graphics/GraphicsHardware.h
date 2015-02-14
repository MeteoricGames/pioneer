// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _GRAPHICSHARDWARE_H_
#define _GRAPHICSHARDWARE_H_

#include "libs.h"

namespace Graphics {

	class Hardware
	{
	public:
		enum ERendererType
		{
			ERT_GL2 = 0,
			ERT_GL3,
		};

		static ERendererType RendererType;

		static bool GL2() { return RendererType == ERT_GL2; }
		static bool GL3() { return RendererType == ERT_GL3; }

		static bool supports_FramebufferObjects;
		static bool supports_sRGBFramebuffers;
		static bool context_CoreProfile;
	};
}

#endif
