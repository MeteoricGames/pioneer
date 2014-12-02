// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _VERTEXBUFFER_GL3_H
#define _VERTEXBUFFER_GL3_H
#include "graphics/VertexBuffer.h"

namespace Graphics { namespace GL3 {

class Effect;

class GLBufferBase {
public:
	GLuint GetBuffer() const { return m_buffer; }

protected:
	GLuint m_buffer;
};

class VertexBuffer : public Graphics::VertexBuffer, public GLBufferBase {
public:
	VertexBuffer(const VertexBufferDesc&);
	~VertexBuffer();

	virtual void Unmap() override;
	virtual void Bind() override;
	virtual void Unbind() override;
	virtual void SetAttribPointers(Effect* effect = nullptr) override;
	virtual void UnsetAttribPointers(Effect* effect = nullptr) override;

protected:
	virtual Uint8 *MapInternal(BufferMapMode, size_t) override;

private:
	Uint8 *m_data;
	bool m_isSetForEffect;
	bool m_isBound;
	GLsizeiptr m_mapRange;
};

class IndexBuffer : public Graphics::IndexBuffer, public GLBufferBase {
public:
	IndexBuffer(Uint32 size, BufferUsage);
	~IndexBuffer();

	virtual Uint32 *Map(BufferMapMode) override;
	virtual void Unmap() override;
	virtual void Bind() override;
	virtual void Unbind() override;

private:
	Uint32 *m_data;
	bool m_isBound;
};

}}

#endif // GL2_VERTEXBUFFER_H
