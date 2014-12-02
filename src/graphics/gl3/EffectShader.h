// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _EFFECT_SHADER_H_
#define _EFFECT_SHADER_H_

#include "libs.h"

namespace Graphics { namespace GL3 {

enum EffectShaderType : unsigned int
{
	EST_FRAGMENT = 0,
	EST_VERTEX,
	EST_GEOMETRY,
};

struct EffectShaderException { };

class EffectShader
{
public:
	EffectShader(
		std::vector<const char*> shader_blocks,
		std::vector<int> shader_sizes,
		EffectShaderType shader_type,
		const std::string& shader_debug_desc);
	virtual ~EffectShader();

	GLuint GetShaderID() const { return m_shader; }

protected:
			
private:
	EffectShader(const EffectShader&);
	EffectShader& operator=(const EffectShader&);

	GLenum m_type;
	GLuint m_shader;
};
	
}} // Namespace

#endif