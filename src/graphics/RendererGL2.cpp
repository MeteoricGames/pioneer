// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "RendererGL2.h"
#include "Graphics.h"
#include "Material.h"
#include "RendererGLBuffers.h"
#include "Texture.h"
#include "TextureGL.h"
#include "VertexArray.h"
#include "gl2/GeoSphereMaterial.h"
#include "gl2/GL2Material.h"
#include "gl2/GL2RenderTarget.h"
#include "gl2/MultiMaterial.h"
#include "gl2/Program.h"
#include "gl2/RingMaterial.h"
#include "gl2/StarfieldMaterial.h"
#include "gl2/FresnelColourMaterial.h"
#include "gl2/TexturedFullscreenQuad.h"
#include "gl2/HorizontalBlurMaterial.h"
#include "gl2/VerticalBlurMaterial.h"
#include "gl2/BloomCompositorMaterial.h"

namespace Graphics {

typedef std::vector<std::pair<MaterialDescriptor, GL2::Program*> >::const_iterator ProgramIterator;

// for material-less line and point drawing
GL2::MultiProgram *vtxColorProg;
GL2::MultiProgram *flatColorProg;

// for post-processing
Material* texFullscreenQuadMtrl = nullptr;
Material* hblurMtrl = nullptr;
Material* vblurMtrl = nullptr;
Material* bloomMtrl = nullptr;
RenderTarget* scenePassRT = nullptr;
RenderTarget* hblurPassRT = nullptr;
RenderTarget* vblurPassRT = nullptr;
GLuint uScreenQuadBufferId = 0;

RendererGL2::RendererGL2(WindowSDL *window, const Graphics::Settings &vs)
: RendererLegacy(window, vs)
, m_invLogZfarPlus1(0.f)
, m_activeRenderTarget(0)
{
	//the range is very large due to a "logarithmic z-buffer" trick used
	//http://outerra.blogspot.com/2009/08/logarithmic-z-buffer.html
	//http://www.gamedev.net/blog/73/entry-2006307-tip-of-the-day-logarithmic-zbuffer-artifacts-fix/
	m_minZNear = 0.0001f;
	m_maxZFar = 10000000.0f;

	MaterialDescriptor desc;
	flatColorProg = new GL2::MultiProgram(desc);
	m_programs.push_back(std::make_pair(desc, flatColorProg));
	desc.vertexColors = true;
	vtxColorProg = new GL2::MultiProgram(desc);
	m_programs.push_back(std::make_pair(desc, vtxColorProg));

	// Init quad used for rendering
	const float screenquad_vertices [] = {
		-1.0f,	-1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
		-1.0f,	 1.0f, 0.0f,
		 1.0f,	-1.0f, 0.0f,
		 1.0f,	 1.0f, 0.0f,
		-1.0f,   1.0f, 0.0f
	};
	glGenBuffers(1, &uScreenQuadBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, uScreenQuadBufferId);
	glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), screenquad_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Init postprocessing materials
	MaterialDescriptor tfquad_mtrl_desc;
	tfquad_mtrl_desc.effect = EFFECT_TEXTURED_FULLSCREEN_QUAD;
	texFullscreenQuadMtrl = CreateMaterial(tfquad_mtrl_desc);
	MaterialDescriptor hblur_mtrl_desc;
	hblur_mtrl_desc.effect = EFFECT_HORIZONTAL_BLUR;
	hblurMtrl = CreateMaterial(hblur_mtrl_desc);
	MaterialDescriptor vblur_mtrl_desc;
	vblur_mtrl_desc.effect = EFFECT_VERTICAL_BLUR;
	vblurMtrl = CreateMaterial(vblur_mtrl_desc);
	MaterialDescriptor bloom_mtrl_desc;
	bloom_mtrl_desc.effect = EFFECT_BLOOM_COMPOSITOR;
	bloomMtrl = CreateMaterial(bloom_mtrl_desc);

	// Init render targets
	RenderTargetDesc scene_rt_desc(
		window->GetWidth(), window->GetHeight(), 
		TextureFormat::TEXTURE_RGBA_8888, TextureFormat::TEXTURE_DEPTH,
		true);
	scenePassRT = CreateRenderTarget(scene_rt_desc);
	RenderTargetDesc hblur_rt_desc(
		window->GetWidth(), window->GetHeight(),
		TextureFormat::TEXTURE_RGB_888, TextureFormat::TEXTURE_NONE,
		false);
	hblurPassRT = CreateRenderTarget(hblur_rt_desc);
	RenderTargetDesc vblur_rt_desc(
		window->GetWidth(), window->GetHeight(),
		TextureFormat::TEXTURE_RGB_888, TextureFormat::TEXTURE_NONE,
		false);
	vblurPassRT = CreateRenderTarget(vblur_rt_desc);
}

RendererGL2::~RendererGL2()
{
	while (!m_programs.empty()) delete m_programs.back().second, m_programs.pop_back();
	if(texFullscreenQuadMtrl) {
		delete texFullscreenQuadMtrl;
	}
	
	glDeleteBuffers(1, &uScreenQuadBufferId);
	if(hblurMtrl) {
		delete hblurMtrl;
	}
	if(vblurMtrl) {
		delete vblurMtrl;
	}
	if(bloomMtrl) {
		delete bloomMtrl;
	}
	if(scenePassRT) {
		delete scenePassRT;
	}
	if(hblurPassRT) {
		delete hblurPassRT;
	}
	if(vblurPassRT) {
		delete vblurPassRT;
	}
}

bool RendererGL2::BeginFrame()
{
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	SetRenderTarget(scenePassRT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return true;
}

bool RendererGL2::EndFrame()
{
	SetRenderTarget(0);
	return true;
}

bool RendererGL2::PostProcessFrame()
{
	glBindBuffer(GL_ARRAY_BUFFER, uScreenQuadBufferId);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// HBlur pass
	SetRenderTarget(hblurPassRT);
	glClear(GL_COLOR_BUFFER_BIT);
	hblurMtrl->texture0 = scenePassRT->GetColorTexture();
	hblurMtrl->Apply();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	hblurMtrl->Unapply();
	// VBlur pass
	SetRenderTarget(vblurPassRT);
	glClear(GL_COLOR_BUFFER_BIT);
	vblurMtrl->texture0 = hblurPassRT->GetColorTexture();
	vblurMtrl->Apply();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	vblurMtrl->Unapply();
	// Combine pass
	SetRenderTarget(0);
	bloomMtrl->texture0 = scenePassRT->GetColorTexture();
	bloomMtrl->texture1 = vblurPassRT->GetColorTexture();
	bloomMtrl->Apply();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	bloomMtrl->Unapply();

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

bool RendererGL2::SetRenderTarget(RenderTarget *rt)
{
	if (m_activeRenderTarget && rt != m_activeRenderTarget)
		m_activeRenderTarget->Unbind();
	if (rt)
		static_cast<GL2::RenderTarget*>(rt)->Bind();

	m_activeRenderTarget = static_cast<GL2::RenderTarget*>(rt);

	return true;
}

bool RendererGL2::SetPerspectiveProjection(float fov, float aspect, float near, float far)
{
	// update values for log-z hack
	m_invLogZfarPlus1 = 1.0f / (log(far+1.0f)/log(2.0f));

	return RendererLegacy::SetPerspectiveProjection(fov, aspect, near, far);
}

bool RendererGL2::SetAmbientColor(const Color &c)
{
	m_ambient = c;
	return true;
}

bool RendererGL2::DrawLines(int count, const vector3f *v, const Color *c, LineType t)
{
	if (count < 2 || !v) return false;

	vtxColorProg->Use();
	vtxColorProg->invLogZfarPlus1.Set(m_invLogZfarPlus1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vector3f), v);
	glColorPointer(4, GL_FLOAT, sizeof(Color), c);
	glDrawArrays(t, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	vtxColorProg->Unuse();

	return true;
}

bool RendererGL2::DrawLines(int count, const vector3f *v, const Color &c, LineType t)
{
	if (count < 2 || !v) return false;

	flatColorProg->Use();
	flatColorProg->diffuse.Set(c);
	flatColorProg->invLogZfarPlus1.Set(m_invLogZfarPlus1);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vector3f), v);
	glDrawArrays(t, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);
	flatColorProg->Unuse();

	return true;
}

Material *RendererGL2::CreateMaterial(const MaterialDescriptor &d)
{
	MaterialDescriptor desc = d;

	GL2::Material *mat = 0;
	GL2::Program *p = 0;

	if (desc.lighting) {
		desc.dirLights = m_numDirLights;
	}

	// Create the material. It will be also used to create the shader,
	// like a tiny factory
	switch (desc.effect) {
	case EFFECT_PLANETRING:
		mat = new GL2::RingMaterial();
		break;
	case EFFECT_STARFIELD:
		mat = new GL2::StarfieldMaterial();
		break;
	case EFFECT_GEOSPHERE_TERRAIN:
	case EFFECT_GEOSPHERE_TERRAIN_WITH_LAVA:
	case EFFECT_GEOSPHERE_TERRAIN_WITH_WATER:
		mat = new GL2::GeoSphereSurfaceMaterial();
		break;
	case EFFECT_GEOSPHERE_SKY:
		mat = new GL2::GeoSphereSkyMaterial();
		break;
	case EFFECT_FRESNEL_SPHERE:
		mat = new GL2::FresnelColourMaterial();
		break;
	case EFFECT_TEXTURED_FULLSCREEN_QUAD:
		mat = new GL2::TexturedFullscreenQuad();
		break;
	case EFFECT_HORIZONTAL_BLUR:
		mat = new GL2::HorizontalBlurMaterial();
		break;
	case EFFECT_VERTICAL_BLUR:
		mat = new GL2::VerticalBlurMaterial();
		break;
	case EFFECT_BLOOM_COMPOSITOR:
		mat = new GL2::BloomCompositorMaterial();
		break;
	default:
		if (desc.lighting)
			mat = new GL2::LitMultiMaterial();
		else
			mat = new GL2::MultiMaterial();
		mat->twoSided = desc.twoSided; //other mats don't care about this
	}

	mat->m_renderer = this;
	mat->m_descriptor = desc;

	try {
		p = GetOrCreateProgram(mat);
	} catch (GL2::ShaderException &) {
		// in release builds, the game does not quit instantly but attempts to revert
		// to a 'shaderless' state
		return RendererLegacy::CreateMaterial(desc);
	}

	mat->SetProgram(p);
	return mat;
}

RenderTarget *RendererGL2::CreateRenderTarget(const RenderTargetDesc &desc)
{
	GL2::RenderTarget* rt = new GL2::RenderTarget(desc);
	rt->Bind();
	if (desc.colorFormat != TEXTURE_NONE) {
		Graphics::TextureDescriptor cdesc(
			desc.colorFormat,
			vector2f(desc.width, desc.height),
			vector2f(desc.width, desc.height),
			LINEAR_CLAMP,
			false,
			false);
		TextureGL *colorTex = new TextureGL(cdesc, false);
		rt->SetColorTexture(colorTex);
	}
	if (desc.depthFormat != TEXTURE_NONE) {
		if (desc.allowDepthTexture) {
			Graphics::TextureDescriptor ddesc(
				TEXTURE_DEPTH,
				vector2f(desc.width, desc.height),
				vector2f(desc.width, desc.height),
				LINEAR_CLAMP,
				false,
				false);
			TextureGL *depthTex = new TextureGL(ddesc, false);
			rt->SetDepthTexture(depthTex);
		} else {
			rt->CreateDepthRenderbuffer();
		}
	}
	rt->CheckStatus();
	rt->Unbind();
	return rt;
}

bool RendererGL2::ReloadShaders()
{
	printf("Reloading " SIZET_FMT " programs...\n", m_programs.size());
	for (ProgramIterator it = m_programs.begin(); it != m_programs.end(); ++it) {
		it->second->Reload();
	}
	printf("Done.\n");

	return true;
}

GL2::Program* RendererGL2::GetOrCreateProgram(GL2::Material *mat)
{
	const MaterialDescriptor &desc = mat->GetDescriptor();
	GL2::Program *p = 0;

	// Find an existing program...
	for (ProgramIterator it = m_programs.begin(); it != m_programs.end(); ++it) {
		if ((*it).first == desc) {
			p = (*it).second;
			break;
		}
	}

	// ...or create a new one
	if (!p) {
		p = mat->CreateProgram(desc);
		m_programs.push_back(std::make_pair(desc, p));
	}

	return p;
}

}
