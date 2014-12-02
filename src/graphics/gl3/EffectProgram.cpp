// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "EffectProgram.h"
#include "EffectShader.h"
#include "Effect.h"

namespace Graphics { namespace GL3 {

GLuint EffectProgram::s_activeProgram = 0;

EffectProgram::EffectProgram(EffectShader* vertex_shader, EffectShader* fragment_shader, 
	const std::string& debug_desc)
{
	// Create program, attach shaders and link
	m_program = glCreateProgram();
	glAttachShader(m_program, vertex_shader->GetShaderID());
	glAttachShader(m_program, fragment_shader->GetShaderID());
    glBindFragDataLocation(m_program, 0, "o_FragColor");
	glLinkProgram(m_program);

	CheckErrors(debug_desc.c_str(), m_program, "program");
}

EffectProgram::~EffectProgram()
{
	glDeleteProgram(m_program);
}

void EffectProgram::Use()
{
	if (s_activeProgram != m_program) {
		glUseProgram(m_program);
		s_activeProgram = m_program;
	}
}

void EffectProgram::Unuse()
{
	glUseProgram(0);
	s_activeProgram = 0;
}

}} // Namespace