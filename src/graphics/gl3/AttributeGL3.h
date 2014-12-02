// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _ATTRIBUTE_GL3_H_
#define _ATTRIBUTE_GL3_H_

#include "libs.h"

namespace Graphics { namespace GL3 {

	class Attribute
	{
	public:
		Attribute();
		virtual ~Attribute();

		void Init(const char *name, GLuint program);

		GLint GetLocation() const { return m_location; }

	private:
		GLint m_location;

	};

}}

#endif
