// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _UNIFORM_GL3_H
#define _UNIFORM_GL3_H
/*
 * Shader uniform
 */
#include "libs.h"
namespace Graphics {

	class Texture;

	namespace GL3 {
		class Uniform {
		public:
			Uniform();
			void Init(const char *name, GLuint program);
			void Set(int);
			void Set(float);
			void Set(const vector2f&);
			void Set(const vector3f&);
			void Set(const vector3d&);
			void Set(const vector4f&);
			void Set(const vector4d&);
			void Set(const Color&);
			void Set(const int v[3]);
			void Set(const float m[9]);
			void Set(const matrix3x3f&);
			void Set(const matrix4x4f&);
			void Set(const float* ptr, unsigned count);
			void Set(const int* ptr, unsigned count);
			void Set(const vector2f* ptr, unsigned count);
			void Set(const vector3f* ptr, unsigned count);
			void Set(const vector4f* ptr, unsigned count);
			void Set(Texture *t, unsigned int unit);

		//private:
			GLint m_location;
		};
	}
}

#endif
