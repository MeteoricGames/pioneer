// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "VertexArray.h"
#include "graphics/gl3/VertexBufferGL3.h"
#include "graphics/Renderer.h"

const int MIN_VB_SIZE = 8;

namespace Graphics {

const unsigned int APosition = ATTRIB_POSITION;
const unsigned int APositionDiffuse = APosition | ATTRIB_DIFFUSE;
const unsigned int APositionDiffuseNormal = APositionDiffuse | ATTRIB_NORMAL;
const unsigned int APositionDiffuseUV = APositionDiffuse | ATTRIB_UV0;
const unsigned int APositionUV = APosition | ATTRIB_UV0;
const unsigned int APositionNormalUV = APosition | ATTRIB_NORMAL | ATTRIB_UV0;

std::map<AttributeSet, GL3::VertexBuffer*> VertexArray::m_vbCache;

VertexArray::VertexArray(AttributeSet attribs, int size) : 
	m_attribs(attribs), m_currentSize(0)
{
	PROFILE_SCOPED()

	if (size > 0) {
		//would be rather weird without positions!
		if (attribs & ATTRIB_POSITION) {
			position.reserve(size);
		}
		if (attribs & ATTRIB_DIFFUSE) {
			diffuse.reserve(size);
		}
		if (attribs & ATTRIB_NORMAL) {
			normal.reserve(size);
		}
		if (attribs & ATTRIB_UV0) {
			uv0.reserve(size);
		}
	}
	InitInternalVB(size);
}

VertexArray::~VertexArray()
{

}

void VertexArray::InitInternalVB(int size)
{
	// Internal Vertex Buffer
	unsigned int vb_size = std::max(size, MIN_VB_SIZE);
	auto vbiter = m_vbCache.find(m_attribs);
	if (vbiter == m_vbCache.end()) {
		// No cache for this attrib set
		VertexBufferDesc vbd;
		memset(vbd.attrib, 0, sizeof(VertexAttribDesc)* 8);
		int cur_atb = 0;
		if (m_attribs & ATTRIB_POSITION) {
			vbd.attrib[cur_atb].semantic = VertexAttrib::ATTRIB_POSITION;
			vbd.attrib[cur_atb].format = ATTRIB_FORMAT_FLOAT4;
			cur_atb++;
		}
		if (m_attribs & ATTRIB_DIFFUSE) {
			vbd.attrib[cur_atb].semantic = VertexAttrib::ATTRIB_DIFFUSE;
			vbd.attrib[cur_atb].format = ATTRIB_FORMAT_UBYTE4;
			cur_atb++;
		}
		if (m_attribs & ATTRIB_NORMAL) {
			vbd.attrib[cur_atb].semantic = VertexAttrib::ATTRIB_NORMAL;
			vbd.attrib[cur_atb].format = ATTRIB_FORMAT_FLOAT4;
			cur_atb++;
		}
		if (m_attribs & ATTRIB_UV0) {
			vbd.attrib[cur_atb].semantic = VertexAttrib::ATTRIB_UV0;
			vbd.attrib[cur_atb].format = ATTRIB_FORMAT_FLOAT2;
			cur_atb++;
		}
		vbd.numVertices = vb_size;
		vbd.stride = 0;
		vbd.usage = BufferUsage::BUFFER_USAGE_DYNAMIC;
		GL3::VertexBuffer* vb = new GL3::VertexBuffer(vbd);
		m_vbCache.insert(std::make_pair(m_attribs, vb));
		m_currentSize = vb_size;
	} else {
		// Cache found, compare sizes
		if (vb_size > vbiter->second->GetVertexCount()) {
			VertexBufferDesc vbd = vbiter->second->GetDesc();
			// Bigger buffer needed, scale to 2x vb_size to minimize future scales
			vb_size *= 2;
			vbd.numVertices = vb_size;
			GL3::VertexBuffer* vb = new GL3::VertexBuffer(vbd);
			delete vbiter->second;
			m_vbCache[m_attribs] = vb;
			m_currentSize = vb_size;
		} else {
			// Size is good enough, carry on
			m_currentSize = vbiter->second->GetVertexCount();
		}
	}
}

bool VertexArray::HasAttrib(VertexAttrib v) const
{
	PROFILE_SCOPED()
	return (m_attribs & v) != 0;
}

unsigned int VertexArray::GetNumVerts() const
{
	PROFILE_SCOPED()
	return position.size();
}

void VertexArray::Clear()
{
	PROFILE_SCOPED()
	position.clear();
	diffuse.clear();
	normal.clear();
	uv0.clear();
}

void VertexArray::Add(const vector2f &v)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v.x, v.y, 0.0f, 1.0f));
}

void VertexArray::Add(const vector3f &v)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v, 1.0f));
}

void VertexArray::Add(const vector2f &v, const Color &c)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v.x, v.y, 0.0f, 1.0f));
	diffuse.push_back(c);
}

void VertexArray::Add(const vector3f &v, const Color &c)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v, 1.0f));
	diffuse.push_back(c);
}

void VertexArray::Add(const vector3f &v, const Color &c, const vector3f &n)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v, 1.0f));
	diffuse.push_back(c);
	normal.push_back(vector4f(n, 0.0f));
}

void VertexArray::Add(const vector3f &v, const Color &c, const vector2f &uv)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v, 1.0f));
	diffuse.push_back(c);
	uv0.push_back(uv);
}

void VertexArray::Add(const vector3f &v, const vector2f &uv)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v, 1.0f));
	uv0.push_back(uv);
}

void VertexArray::Add(const vector3f &v, const vector3f &n, const vector2f &uv)
{
	PROFILE_SCOPED()
	position.push_back(vector4f(v, 1.0f));
	normal.push_back(vector4f(n, 0.0f));
	uv0.push_back(uv);
}

void VertexArray::Set(const Uint32 idx, const vector3f &v)
{
	PROFILE_SCOPED()
	position[idx] = v;
}

void VertexArray::Set(const Uint32 idx, const vector3f &v, const Color &c)
{
	PROFILE_SCOPED()
	position[idx] = v;
	diffuse[idx] = c;
}

void VertexArray::Set(const Uint32 idx, const vector3f &v, const Color &c, const vector3f &n)
{
	PROFILE_SCOPED()
	position[idx] = v;
	diffuse[idx] = c;
	normal[idx] = n;
}

void VertexArray::Set(const Uint32 idx, const vector3f &v, const Color &c, const vector2f &uv)
{
	PROFILE_SCOPED()
	position[idx] = v;
	diffuse[idx] = c;
	uv0[idx] = uv;
}

void VertexArray::Set(const Uint32 idx, const vector3f &v, const vector2f &uv)
{
	PROFILE_SCOPED()
	position[idx] = v;
	uv0[idx] = uv;
}

void VertexArray::Set(const Uint32 idx, const vector3f &v, const vector3f &n, const vector2f &uv)
{
	PROFILE_SCOPED()
	position[idx] = v;
	normal[idx] = n;
	uv0[idx] = uv;
}

void VertexArray::UpdateInternalVB()
{
	if(m_currentSize < position.size()) {
		InitInternalVB(position.size());
	}
	GL3::VertexBuffer* vb = GetVB();
	assert(vb);
	Uint8* vtx_ptr = vb->Map<Uint8>(Graphics::BufferMapMode::BUFFER_MAP_WRITE, position.size());
	switch(m_attribs) {
		case APosition:
			_uvb_position(vtx_ptr);
			break;

		case APositionDiffuse:
			_uvb_positionDiffuse(vtx_ptr);
			break;

		case APositionDiffuseNormal:
			_uvb_positionDiffuseNormal(vtx_ptr);
			break;

		case APositionDiffuseUV:
			_uvb_positionDiffuseUV(vtx_ptr);
			break;

		case APositionUV:
			_uvb_positionUV(vtx_ptr);
			break;

		case APositionNormalUV:
			_uvb_positionNormalUV(vtx_ptr);
			break;
	}
	vb->Unmap();
}

#define _UVB_BEGIN	for(unsigned i = 0; i < position.size(); ++i) {
#define _UVB_END	}

#define _UVB_COPY_POSITION(a)	memcpy(a, &position[i], 16);	a += 16;
#define _UVB_COPY_NORMAL(a)		memcpy(a, &normal[i],	16);	a += 16;
#define _UVB_COPY_DIFFUSE(a)	memcpy(a, &diffuse[i],	4);		a += 4;
#define _UVB_COPY_UV0(a)		memcpy(a, &uv0[i],		8);		a += 8;

void VertexArray::_uvb_position(Uint8* vp)
{
	_UVB_BEGIN
		_UVB_COPY_POSITION(vp);
	_UVB_END;
}
void VertexArray::_uvb_positionDiffuse(Uint8* vp)
{
	_UVB_BEGIN
		_UVB_COPY_POSITION(vp);
		_UVB_COPY_DIFFUSE(vp);
	_UVB_END;
}
void VertexArray::_uvb_positionDiffuseNormal(Uint8* vp)
{
	_UVB_BEGIN
		_UVB_COPY_POSITION(vp);
		_UVB_COPY_DIFFUSE(vp);
		_UVB_COPY_NORMAL(vp);
	_UVB_END;
}
void VertexArray::_uvb_positionDiffuseUV(Uint8* vp)
{
	_UVB_BEGIN
		_UVB_COPY_POSITION(vp);
		_UVB_COPY_DIFFUSE(vp);
		_UVB_COPY_UV0(vp);
	_UVB_END;
}
void VertexArray::_uvb_positionUV(Uint8* vp)
{
	_UVB_BEGIN
		_UVB_COPY_POSITION(vp);
		_UVB_COPY_UV0(vp);
	_UVB_END;
}
void VertexArray::_uvb_positionNormalUV(Uint8* vp)
{
	_UVB_BEGIN
		_UVB_COPY_POSITION(vp);
		_UVB_COPY_NORMAL(vp);
		_UVB_COPY_UV0(vp);
	_UVB_END;
}

}
