// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "GuiTexturedQuad.h"
#include "graphics/Renderer.h"
#include "graphics/Material.h"
#include "graphics/VertexArray.h"
#include "GuiScreen.h"
#include "MainMaterial.h"

using namespace Graphics;

namespace Gui {

TexturedQuad::TexturedQuad(Renderer* renderer, Graphics::Texture *texture) : 
	m_renderer(renderer), m_texture(RefCountedPtr<Graphics::Texture>(texture)) 
{
	Graphics::MaterialDescriptor desc;
	desc.textures = 1;
	if (Graphics::Hardware::GL3()) {
		m_material.reset(new MainMaterial(renderer, desc));
	} else {
		m_material.reset(renderer->CreateMaterial(desc));
	}
	m_material->texture0 = m_texture.Get();

	m_va.reset(new Graphics::VertexArray(ATTRIB_POSITION | ATTRIB_UV0));
	vector2f pos(0.0f, 0.0f);
	vector2f size(1.0f, 1.0f);
	vector2f texPos(0.0f, 0.0f);
	vector2f texSize(1.0f, 1.0f);
	m_va->Add(vector3f(pos.x, pos.y, 0.0f), vector2f(texPos.x, texPos.y));
	m_va->Add(vector3f(pos.x, pos.y + size.y, 0.0f), vector2f(texPos.x, texPos.y + texSize.y));
	m_va->Add(vector3f(pos.x + size.x, pos.y, 0.0f), vector2f(texPos.x + texSize.x, texPos.y));
	m_va->Add(vector3f(pos.x + size.x, pos.y + size.y, 0.0f), vector2f(texPos.x + texSize.x, texPos.y + texSize.y));
}

void TexturedQuad::Draw(const vector2f &pos, const vector2f &size, const vector2f &texPos, 
	const vector2f &texSize, const Color &tint)
{
	PROFILE_SCOPED()
	
	m_va->Set(0, vector3f(pos.x,        pos.y,        0.0f), vector2f(texPos.x,           texPos.y));
	m_va->Set(1, vector3f(pos.x,        pos.y+size.y, 0.0f), vector2f(texPos.x,           texPos.y+texSize.y));
	m_va->Set(2, vector3f(pos.x+size.x, pos.y,        0.0f), vector2f(texPos.x+texSize.x, texPos.y));
	m_va->Set(3, vector3f(pos.x+size.x, pos.y+size.y, 0.0f), vector2f(texPos.x+texSize.x, texPos.y+texSize.y));

	m_material->diffuse = tint;
	m_renderer->DrawTriangles(m_va.get(), Gui::Screen::alphaBlendState, m_material.get(), TRIANGLE_STRIP);
}

}
