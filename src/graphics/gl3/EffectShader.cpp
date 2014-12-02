// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "EffectShader.h"
#include "Effect.h"
#include "utils.h"

namespace Graphics { namespace GL3 {

EffectShader::EffectShader(
	std::vector<const char*> shader_blocks,
	std::vector<int> shader_sizes,
	EffectShaderType shader_type,
	const std::string& shader_debug_desc)
{
	assert(shader_type != EffectShaderType::EST_GEOMETRY); // Not supported yet
	assert(shader_blocks.size() > 2 && shader_sizes.size() == shader_blocks.size()); // faulty data given
	m_type = shader_type == EffectShaderType::EST_VERTEX? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
	m_shader = glCreateShader(m_type);
	glShaderSource(m_shader, shader_blocks.size(), &shader_blocks[0], &shader_sizes[0]);
	glCompileShader(m_shader);

	if (!CheckErrors(shader_debug_desc.c_str(), m_shader)) {
		throw EffectShaderException();
	}
}

EffectShader::~EffectShader()
{
	glDeleteShader(m_shader);
}


}} // Namespace
