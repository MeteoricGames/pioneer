// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Renderer.h"
#include "Texture.h"
#include "PostProcessing.h"

namespace Graphics {

Renderer::Renderer(WindowSDL *window, int w, int h) :
	m_width(w), m_height(h), m_ambient(Color::BLACK), m_window(window)
{
	m_lineWidth = 1.0f;
	LightsState ls;
	m_lightsStack.push(ls);
	m_viewportStack.push(ViewportState());
	m_scissorStack.push(ScissorState());
	m_currentViewTransform = matrix4x4f::Identity();
}

Renderer::~Renderer()
{
	RemoveAllCachedTextures();
}

Texture *Renderer::GetCachedTexture(const std::string &type, const std::string &name)
{
	TextureCacheMap::iterator i = m_textures.find(TextureCacheKey(type,name));
	if (i == m_textures.end()) return 0;
	return (*i).second->Get();
}

void Renderer::AddCachedTexture(const std::string &type, const std::string &name, Texture *texture)
{
	RemoveCachedTexture(type,name);
	m_textures.insert(std::make_pair(TextureCacheKey(type,name),new RefCountedPtr<Texture>(texture)));
}

void Renderer::RemoveCachedTexture(const std::string &type, const std::string &name)
{
	TextureCacheMap::iterator i = m_textures.find(TextureCacheKey(type,name));
	if (i == m_textures.end()) return;
	delete (*i).second;
	m_textures.erase(i);
}

void Renderer::RemoveAllCachedTextures()
{
	for (TextureCacheMap::iterator i = m_textures.begin(); i != m_textures.end(); ++i)
		delete (*i).second;
	m_textures.clear();
}

void Renderer::SetLineWidth(float width)
{
	m_lineWidth = width; 
	glLineWidth(width);
}

void Renderer::PushLightsState(int num_lights, const Light* lights)
{
	m_lightsStack.push(LightsState());
	SetLights(num_lights, lights);
}

void Renderer::PopLightsState() 
{
	assert(m_lightsStack.size() > 1);
	if(m_lightsStack.size() > 1) {
		m_lightsStack.pop();
		SetLights(m_lightsStack.top().numLights, m_lightsStack.top().lights);
	}
}

void Renderer::PushViewportState(Sint32 x, Sint32 y, Sint32 width, Sint32 height)
{
	m_viewportStack.push(ViewportState());
	SetViewport(x, y, width, height);
}

void Renderer::PopViewportState()
{
	assert(m_viewportStack.size() > 1);
	if(m_viewportStack.size() > 1) {
		m_viewportStack.pop();
		ViewportState& vs = m_viewportStack.top();
		SetViewport(vs.x, vs.y, vs.w, vs.h);
	}
}

void Renderer::PushScissorState(bool enable_scissor, const vector2f &position, const vector2f &size) {
	m_scissorStack.push(ScissorState());
	SetScissor(enable_scissor, position, size);
}

void Renderer::PopScissorState()
{
	assert(m_scissorStack.size() > 1);
	if(m_scissorStack.size() > 1) {
		m_scissorStack.pop();
		ScissorState &ss = m_scissorStack.top();
		SetScissor(ss.enable, ss.position, ss.size);
	}
}

}
