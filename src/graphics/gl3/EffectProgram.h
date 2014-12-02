// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _EFFECT_PROGRAM_H_
#define _EFFECT_PROGRAM_H_

#include "libs.h"

namespace Graphics { namespace GL3 {

class EffectShader;

class EffectProgram
{
public:
	EffectProgram(EffectShader* vertex_shader, EffectShader* fragment_shader, 
		const std::string& debug_desc);
	virtual ~EffectProgram();

	void Use();
	void Unuse();

	GLuint GetProgramID() const { return m_program; }

protected:

private:
	EffectProgram(const EffectProgram&);
	EffectProgram& operator=(const EffectProgram&);

	GLuint m_program;

private: // Static
	static GLuint s_activeProgram;

};

}} // Namespace


#endif