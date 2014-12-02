// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "AttributeGL3.h"

namespace Graphics { namespace GL3 {

Attribute::Attribute() :
	m_location(-1)
{

}

Attribute::~Attribute()
{

}

void Attribute::Init(const char *name, GLuint program)
{
	m_location = glGetAttribLocation(program, name);
}

}}
