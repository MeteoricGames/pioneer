// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef GRAPHICS_VERTEXBUFFER_H
#define GRAPHICS_VERTEXBUFFER_H
/**
 * A Vertex Buffer is created by filling out a
 * description struct with desired vertex attributes
 * and calling renderer->CreateVertexBuffer.
 * Can be used in combination with IndexBuffer,
 * for optimal rendering of complex geometry.
 * Call Map to write/read from the buffer, and Unmap to
 * commit the changes.
 * Buffers come in two usage flavors, static and dynamic.
 * Use Static buffer, when the geometry never changes.
 * Avoid mapping a buffer for reading, as it may be slow,
 * especially with static buffers.
 *
 * Expansion possibilities: range-based Map
 */
#include "libs.h"
#include "graphics/Types.h"

namespace Graphics {

	namespace GL3 { class Effect; }

const Uint32 MAX_ATTRIBS = 8;

struct VertexAttribDesc {
	//position, texcoord, normal etc.
	VertexAttrib semantic;
	//float3, float2 etc.
	VertexAttribFormat format;
	//byte offset of the attribute, if zero this
	//is automatically filled for created buffers
	Uint32 offset;
};

struct VertexBufferDesc {
	VertexBufferDesc();
	//byte offset of an existing attribute
	Uint32 GetOffset(VertexAttrib) const;

	//used internally for calculating offsets
	static Uint32 CalculateOffset(const VertexBufferDesc&, VertexAttrib);
	static Uint32 GetAttribSize(VertexAttribFormat);

	//semantic ATTRIB_NONE ends description (when not using all attribs)
	VertexAttribDesc attrib[MAX_ATTRIBS];
	Uint32 numVertices;
	//byte size of one vertex, if zero this is
	//automatically calculated for created buffers
	Uint32 stride;
	BufferUsage usage;
};

class Mappable {
public:
	virtual ~Mappable() { }
	virtual void Unmap() = 0;

protected:
	Mappable() : m_mapMode(BUFFER_MAP_NONE) { }
	BufferMapMode m_mapMode; //tracking map state
};

class VertexBuffer : public RefCounted, public Mappable {
public:
	virtual ~VertexBuffer();
	const VertexBufferDesc &GetDesc() const { return m_desc; }

	template <typename T> T *Map(BufferMapMode mode, size_t vcount = 0) {
		return reinterpret_cast<T*>(MapInternal(mode, vcount));
	}

	//Vertex count used for rendering.
	//By default the maximum set in description, but
	//you may set a smaller count for partial rendering
	Uint32 GetVertexCount() const;
	void SetVertexCount(Uint32);

	// Binds buffer for rendering (required for GL3 only)
	virtual void Bind() {}
	virtual void Unbind() {}

	// Used to set arrays for rendering
	virtual void SetAttribPointers(GL3::Effect* effect = nullptr) { assert(false); }
	virtual void UnsetAttribPointers(GL3::Effect* effect = nullptr) { assert(false); }

protected:
	virtual Uint8 *MapInternal(BufferMapMode, size_t) = 0;
	VertexBufferDesc m_desc;
	Uint32 m_numVertices;
};

// Index buffer, limited to Uint32 index format for better portability
class IndexBuffer : public RefCounted, public Mappable {
public:
	IndexBuffer(Uint32 size, BufferUsage);
	virtual ~IndexBuffer();

	virtual Uint32 *Map(BufferMapMode) = 0;

	Uint32 GetSize() const { return m_size; }
	Uint32 GetIndexCount() const { return m_indexCount; }
	void SetIndexCount(Uint32);
	BufferUsage GetUsage() const { return m_usage; }

	// Binds buffer for rendering
	virtual void Bind() {}
	virtual void Unbind() {}

protected:
	Uint32 m_size;
	Uint32 m_indexCount;
	BufferUsage m_usage;
};

}
#endif // GRAPHICS_VERTEXBUFFER_H
