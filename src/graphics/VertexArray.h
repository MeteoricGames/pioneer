// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _VERTEXARRAY_H
#define _VERTEXARRAY_H

#include "libs.h"
#include "graphics/Types.h"

namespace Graphics {
	class Renderer;
	namespace GL3 {
		class VertexBuffer;
	};

typedef unsigned int AttributeSet;

/*
 * VertexArray is a multi-purpose vertex container. Users specify
 * the attributes they intend to use and then add vertices. Renderers
 * do whatever they need to do with regards to the attribute set.
 * This is not optimized for high performance drawing, but okay for simple
 * cases.
 *
 * Salwan: Upgrade to GL3
 * VertexArray is a tricky one since it's used sporadically and directly in the UI code
 * The whole method of drawing direct arrays is deprecated. Everything goes in a vertex
 * buffer from now on.
 * To solve this my solution is to internally use VBOs inside VertexArray while maintaining
 * the exact same interface to UI. So while UI directly draws, internally VertexArray will
 * cache, map and set vertex buffers. Which will consequently be used for drawing the actual
 * geometry. This will be done for both GL2 and GL3.
 */
class VertexArray {
	friend class Renderer;
	friend class RendererGL2;
	friend class RendererGL3;
public:
	//specify attributes to be used, additionally reserve space for vertices
	VertexArray(AttributeSet attribs, int size=0);
	~VertexArray();

	//check presence of an attribute
	bool HasAttrib(VertexAttrib v) const;
	unsigned int GetNumVerts() const;
	AttributeSet GetAttributeSet() const { return m_attribs; }

	//removes vertices, does not deallocate space
	void Clear();

	// don't mix these
	void Add(const vector2f &v);
	void Add(const vector3f &v);
	void Add(const vector2f &v, const Color &c);
	void Add(const vector3f &v, const Color &c);
	void Add(const vector3f &v, const Color &c, const vector3f &normal);
	void Add(const vector3f &v, const Color &c, const vector2f &uv);
	void Add(const vector3f &v, const vector2f &uv);
	void Add(const vector3f &v, const vector3f &normal, const vector2f &uv);
	//virtual void Reserve(unsigned int howmuch)

	// don't mix these
	void Set(const Uint32 idx, const vector3f &v);
	void Set(const Uint32 idx, const vector3f &v, const Color &c);
	void Set(const Uint32 idx, const vector3f &v, const Color &c, const vector3f &normal);
	void Set(const Uint32 idx, const vector3f &v, const Color &c, const vector2f &uv);
	void Set(const Uint32 idx, const vector3f &v, const vector2f &uv);
	void Set(const Uint32 idx, const vector3f &v, const vector3f &normal, const vector2f &uv);

	//could make these private, but it is nice to be able to
	//add attributes separately...
	std::vector<vector4f> position;
	std::vector<vector4f> normal;
	std::vector<Color> diffuse;
	std::vector<vector2f> uv0;

private:

	void InitInternalVB(int size);
	void UpdateInternalVB();
	
	inline void _uvb_position(Uint8* vp);
	inline void _uvb_positionDiffuse(Uint8* vp);
	inline void _uvb_positionDiffuseNormal(Uint8* vp);
	inline void _uvb_positionDiffuseUV(Uint8* vp);
	inline void _uvb_positionUV(Uint8* vp);
	inline void _uvb_positionNormalUV(Uint8* vp);

	GL3::VertexBuffer* GetVB() { return m_vbCache[m_attribs]; }

	AttributeSet m_attribs;
	unsigned int m_currentSize;

	// Statics
	static std::map<AttributeSet, GL3::VertexBuffer*> m_vbCache;
};

}

#endif
