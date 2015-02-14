// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "graphics/gl3/VertexBufferGL3.h"
#include "graphics/gl3/Effect.h"

namespace Graphics { namespace GL3 {

GLint get_num_components(VertexAttribFormat fmt)
{
	switch (fmt) {
	case ATTRIB_FORMAT_FLOAT2:
		return 2;
	case ATTRIB_FORMAT_FLOAT3:
		return 3;
	case ATTRIB_FORMAT_FLOAT4:
	case ATTRIB_FORMAT_UBYTE4:
		return 4;
	default:
		assert(false);
		return 0;
	}
}

GLenum get_component_type(VertexAttribFormat fmt)
{
	switch (fmt) {
	case ATTRIB_FORMAT_UBYTE4:
		return GL_UNSIGNED_BYTE;
	case ATTRIB_FORMAT_FLOAT2:
	case ATTRIB_FORMAT_FLOAT3:
	case ATTRIB_FORMAT_FLOAT4:
	default:
		return GL_FLOAT;
	}
}

VertexBuffer::VertexBuffer(const VertexBufferDesc &desc)
{
	m_desc = desc;
	m_isSetForEffect = false;
	m_isBound = false;
	m_mapRange = 0;
	//update offsets in desc
	for (Uint32 i = 0; i < MAX_ATTRIBS; i++) {
		if (m_desc.attrib[i].offset == 0)
			m_desc.attrib[i].offset = VertexBufferDesc::CalculateOffset(m_desc, m_desc.attrib[i].semantic);
	}

	//update stride in desc (respecting offsets)
	if (m_desc.stride == 0)
	{
		Uint32 lastAttrib = 0;
		while (lastAttrib < MAX_ATTRIBS) {
			if (m_desc.attrib[lastAttrib].semantic == ATTRIB_NONE)
				break;
			lastAttrib++;
		}

		m_desc.stride = m_desc.attrib[lastAttrib].offset + VertexBufferDesc::GetAttribSize(m_desc.attrib[lastAttrib].format);
	}
	assert(m_desc.stride > 0);
	assert(m_desc.numVertices > 0);

	SetVertexCount(m_desc.numVertices);

	glGenBuffers(1, &m_buffer);

	//Allocate initial data store
	//Using zeroed m_data is not mandatory, but otherwise contents are undefined
	glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
	const Uint32 dataSize = m_desc.numVertices * m_desc.stride;
	m_data = new Uint8[dataSize];
	memset(m_data, 0, dataSize);
	const GLenum usage = (m_desc.usage == BUFFER_USAGE_STATIC) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	glBufferData(GL_ARRAY_BUFFER, dataSize, m_data, usage);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Don't keep client data around for static buffers
	if (GetDesc().usage == BUFFER_USAGE_STATIC) {
		delete[] m_data;
		m_data = nullptr;
	}

	//If we had VAOs could set up the pointers already
}

VertexBuffer::~VertexBuffer()
{
	glDeleteBuffers(1, &m_buffer);
	delete[] m_data;
}

Uint8 *VertexBuffer::MapInternal(BufferMapMode mode, size_t vcount)
{
	assert(mode != BUFFER_MAP_NONE); //makes no sense
	assert(m_mapMode == BUFFER_MAP_NONE); //must not be currently mapped
	m_mapMode = mode;
	if (GetDesc().usage == BUFFER_USAGE_STATIC) {
		glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
		if(vcount == 0 || vcount == GetVertexCount()) {
			if (mode == BUFFER_MAP_READ) {
				return reinterpret_cast<Uint8*>(
					glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
			} else if (mode == BUFFER_MAP_WRITE) {
				return reinterpret_cast<Uint8*>(
					glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
			}
		} else {
			m_mapRange = vcount * m_desc.stride;
			if(mode == BUFFER_MAP_READ) {
				return reinterpret_cast<Uint8*>(
					glMapBufferRange(GL_ARRAY_BUFFER, 0, m_mapRange, GL_READ_ONLY));
			} else if (mode == BUFFER_MAP_WRITE) {
				return reinterpret_cast<Uint8*>(
					glMapBufferRange(GL_ARRAY_BUFFER, 0, m_mapRange, GL_WRITE_ONLY));
			}
		}
	} else if(vcount != 0 && vcount != GetVertexCount()) {
		m_mapRange = vcount * m_desc.stride;
	}
	return m_data;
}

void VertexBuffer::Unmap()
{
	assert(m_mapMode != BUFFER_MAP_NONE); //not currently mapped

	if (GetDesc().usage == BUFFER_USAGE_STATIC) {
		glUnmapBuffer(GL_ARRAY_BUFFER);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
	} else {
		if (m_mapMode == BUFFER_MAP_WRITE) {
			GLsizei dataSize;
			if(m_mapRange == 0) {
				dataSize = m_desc.numVertices * m_desc.stride;
			} else {
				dataSize = m_mapRange;
			}
			glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, m_data);
			//glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
	m_mapRange = 0;
	m_mapMode = BUFFER_MAP_NONE;
}

void VertexBuffer::Bind()
{
	assert(!m_isBound);
	glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
	m_isBound = true;
}

void VertexBuffer::Unbind()
{
	assert(m_isBound);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_isBound = false;
}

inline EEffectAttributes GetEffectAttribute(VertexAttrib semantic)
{
	switch (semantic) {
		case ATTRIB_POSITION:
			return EEffectAttributes::EEA_POSITION;

		case ATTRIB_NORMAL:
			return EEffectAttributes::EEA_NORMAL;

		case ATTRIB_DIFFUSE:
			return EEffectAttributes::EEA_DIFFUSE;

		case ATTRIB_UV0:
			return EEffectAttributes::EEA_TEXCOORDS0;

		case ATTRIB_NONE:
		default:
			assert(false);
			return EEffectAttributes::EEA_TOTAL;
	}
}

void VertexBuffer::SetAttribPointers(Effect* effect)
{
	assert(effect);
	for(Uint8 i = 0; i < MAX_ATTRIBS; i++) {
		VertexAttrib s = m_desc.attrib[i].semantic;
		if (s == ATTRIB_NONE) {
			return;
		}
		EEffectAttributes at = GetEffectAttribute(s);
		int al = effect->GetAttribute(at).GetLocation();
		int ai = static_cast<int>(at);
		auto offset = reinterpret_cast<const GLvoid*>(m_desc.attrib[i].offset);

		if(al < 0) {
			// Unused attributes get culled by GLSL compilers regardless of situation, should carry on
			// as usual even if an attribute is not found. It'll be skipped by stride and correct data
			// is sent to GPU.
			//
			//Error("VertexBuffer GL3 (set): Attribute (%s) not found in effect: %s",
			//	EffectAttributes[static_cast<unsigned>(at)].c_str(), effect->GetDebugString());
			continue;
		} else {
			glEnableVertexAttribArray(al);
			bool normalized = false;
			unsigned type = GL_FLOAT;
			// For diffuse, we want 4 ubyte to go into the GPU and gets normalized to 0-1 range for each component
			if (at == EEffectAttributes::EEA_DIFFUSE) {
				type = GL_UNSIGNED_BYTE;
				normalized = true;
			}
			glVertexAttribPointer(al, EffectAttributesSizes[ai], type, normalized, m_desc.stride, offset);
		}
		m_isSetForEffect = true;
	}
}

void VertexBuffer::UnsetAttribPointers(Effect* effect)
{
	assert(effect);
	assert(m_isSetForEffect);
	if(m_isSetForEffect) {
		m_isSetForEffect = false;
		for(Uint8 i = 0; i < MAX_ATTRIBS; i++) {
			VertexAttrib s = m_desc.attrib[i].semantic;
			if(s == ATTRIB_NONE) {
				return;
			}
			int a = effect->GetAttribute(GetEffectAttribute(s)).GetLocation();
			if (a < 0) {
				//Error("VertexBuffer GL3 (unset): Attribute not found in effect %d", a);
				continue;
			} else {
				glDisableVertexAttribArray(a);
			}
		}
	}
}

IndexBuffer::IndexBuffer(Uint32 size, BufferUsage hint)
	: Graphics::IndexBuffer(size, hint), m_isBound(false)
{
	assert(size > 0);

	const GLenum usage = (hint == BUFFER_USAGE_STATIC) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	glGenBuffers(1, &m_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffer);
	m_data = new Uint32[size];
	memset(m_data, 0, sizeof(Uint32) * size);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Uint32) * m_size, m_data, usage);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//Don't keep client data around for static buffers
	if (GetUsage() == BUFFER_USAGE_STATIC) {
		delete[] m_data;
		m_data = nullptr;
	}
}

IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_buffer);
	delete[] m_data;
}

Uint32 *IndexBuffer::Map(BufferMapMode mode)
{
	assert(mode != BUFFER_MAP_NONE); //makes no sense
	assert(m_mapMode == BUFFER_MAP_NONE); //must not be currently mapped
	m_mapMode = mode;
	if (GetUsage() == BUFFER_USAGE_STATIC) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffer);
		if (mode == BUFFER_MAP_READ)
			return reinterpret_cast<Uint32*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY));
		else if (mode == BUFFER_MAP_WRITE)
			return reinterpret_cast<Uint32*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
	}

	return m_data;
}

void IndexBuffer::Unmap()
{
	assert(m_mapMode != BUFFER_MAP_NONE); //not currently mapped

	if (GetUsage() == BUFFER_USAGE_STATIC) {
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	} else {
		if (m_mapMode == BUFFER_MAP_WRITE) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffer);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(Uint32) * m_size, m_data);
			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}

	m_mapMode = BUFFER_MAP_NONE;
}

void IndexBuffer::Bind()
{
	assert(!m_isBound);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GetBuffer());
	m_isBound = true;
}

void IndexBuffer::Unbind()
{
	assert(m_isBound);
	m_isBound = false;
}


} }
