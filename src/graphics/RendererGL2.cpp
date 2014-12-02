// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "RendererGL2.h"
#include "Graphics.h"
#include "Light.h"
#include "Material.h"
#include "OS.h"
#include "StringF.h"
#include "Texture.h"
#include "TextureGL.h"
#include "VertexArray.h"
#include "PostProcessing.h"
#include "GLDebug.h"
#include "gl2/GeoSphereMaterial.h"
#include "gl2/GL2Material.h"
#include "gl2/GL2RenderState.h"
#include "gl2/GL2RenderTarget.h"
#include "gl2/GL2VertexBuffer.h"
#include "gl2/MultiMaterial.h"
#include "gl2/Program.h"
#include "gl2/RingMaterial.h"
#include "gl2/StarfieldMaterial.h"
#include "gl2/FresnelColourMaterial.h"
#include "gl2/ShieldMaterial.h"
#include "gl2/SkyboxMaterial.h"
#include "gl2/TexturedFullscreenQuad.h"
#include "gl2/HorizontalBlurMaterial.h"
#include "gl2/VerticalBlurMaterial.h"
#include "gl2/SphereImpostorMaterial.h"
#include "gl2/BloomCompositorMaterial.h"
#include "effects/thruster_trails/ThrusterTrailsDepthMaterial.h"
#include "effects/thruster_trails/ThrusterTrailsMaterial.h"
#include "effects/sector_view/SectorViewIconMaterial.h"
#include "effects/transit/TransitEffectMaterial.h"
#include "effects/transit/TransitCompositionMaterial.h"
#include "effects/RadialBlurMaterial.h"
#include <stddef.h> //for offsetof
#include <ostream>
#include <sstream>
#include <iterator>

namespace Graphics {

typedef std::vector<std::pair<MaterialDescriptor, GL2::Program*> >::const_iterator ProgramIterator;

// for material-less line and point drawing
GL2::MultiProgram *vtxColorProg;
GL2::MultiProgram *flatColorProg;
GL2::MultiProgram *pointsColorProg;

RendererGL2::RendererGL2(WindowSDL *window, const Graphics::Settings &vs)
: Renderer(window, window->GetWidth(), window->GetHeight())
//the range is very large due to a "logarithmic z-buffer" trick used
//http://outerra.blogspot.com/2009/08/logarithmic-z-buffer.html
//http://www.gamedev.net/blog/73/entry-2006307-tip-of-the-day-logarithmic-zbuffer-artifacts-fix/
, m_minZNear(0.0001f)
, m_maxZFar(10000000.0f)
, m_useCompressedTextures(false)
, m_activeRenderTarget(nullptr)
, m_activeRenderState(nullptr)
, m_matrixMode(MatrixMode::MODELVIEW)
{
	m_invLogZfarPlus1 = 0.0f;

	const bool useDXTnTextures = vs.useTextureCompression && glewIsSupported("GL_EXT_texture_compression_s3tc");
	m_useCompressedTextures = useDXTnTextures;

	//XXX bunch of fixed function states here!
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHT0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glMatrixMode(GL_MODELVIEW);
	m_modelViewStack.push(matrix4x4f::Identity());
	m_projectionStack.push(matrix4x4f::Identity());

	SetClearColor(Color(0.f));
	SetViewport(0, 0, m_width, m_height);

	if (vs.enableDebugMessages)
		GLDebug::Enable();

	MaterialDescriptor desc;
	flatColorProg = new GL2::MultiProgram(desc);
	m_programs.push_back(std::make_pair(desc, flatColorProg));
	desc.vertexColors = true;
	vtxColorProg = new GL2::MultiProgram(desc);
	m_programs.push_back(std::make_pair(desc, vtxColorProg));
	desc.pointsMode = true;
	pointsColorProg = new GL2::MultiProgram(desc);
	m_programs.push_back(std::make_pair(desc, pointsColorProg));

	// Init fullscreen quad
	/*const vector2f pixel_size = vector2f(
		1.0f / static_cast<float>(window->GetWidth()), 
		1.0f / static_cast<float>(window->GetHeight()));
	const float screenquad_vertices[] = {
		-1.0f + pixel_size.x,	-1.0f + pixel_size.y, 0.0f,
		 1.0f + pixel_size.x,	-1.0f + pixel_size.y, 0.0f,
		-1.0f + pixel_size.x,	 1.0f + pixel_size.y, 0.0f,
		 1.0f + pixel_size.x,	-1.0f + pixel_size.y, 0.0f,
		 1.0f + pixel_size.x,	 1.0f + pixel_size.y, 0.0f,
		-1.0f + pixel_size.x,	 1.0f + pixel_size.y, 0.0f
	};*/
	const float screenquad_vertices[] = {
		-1.0f,	-1.0f,	0.0f,
		 1.0f,	-1.0f,	0.0f,
		-1.0f,	 1.0f,	0.0f,
		 1.0f,	-1.0f,	0.0f,
		 1.0f,	 1.0f,	0.0f,
		-1.0f,	 1.0f,	0.0f
	};
	glGenBuffers(1, &m_screenQuadBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, m_screenQuadBufferId);
	glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), screenquad_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	RenderStateDesc sqrs;
	sqrs.blendMode = BlendMode::BLEND_SOLID;
	sqrs.depthTest = false;
	sqrs.depthWrite = false;
	m_screenQuadRS = CreateRenderState(sqrs);
	MaterialDescriptor sqmd;
	sqmd.effect = EFFECT_TEXTURED_FULLSCREEN_QUAD;
	m_screenQuadMtrl.reset(CreateMaterial(sqmd));

	// Init post processing
	m_postprocessing.reset(new PostProcessing(this));
}

RendererGL2::~RendererGL2()
{
	while (!m_programs.empty()) delete m_programs.back().second, m_programs.pop_back();
	for (auto state : m_renderStates)
		delete state.second;
}

bool RendererGL2::GetNearFarRange(float &near, float &far) const
{
	near = m_minZNear;
	far = m_maxZFar;
	return true;
}

bool RendererGL2::BeginFrame()
{
	PROFILE_SCOPED()
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return true;
}

bool RendererGL2::BeginPostProcessing(RenderTarget* rt_device)
{
	m_postprocessing->SetDeviceRT(rt_device);
	m_postprocessing->BeginFrame();
	return true;
}

bool RendererGL2::PostProcessFrame(PostProcess* postprocess)
{	
	m_postprocessing->Run(postprocess);
	return true;
}

bool RendererGL2::EndPostProcessing()
{
	m_postprocessing->EndFrame();
	return true;
}

bool RendererGL2::EndFrame()
{
	return true;
}

bool RendererGL2::SwapBuffers()
{
	PROFILE_SCOPED()
#ifndef NDEBUG
	// Check if an error occurred during the frame. This is not very useful for
	// determining *where* the error happened. For that purpose, try GDebugger or
	// the GL_KHR_DEBUG extension
	GLenum err;
	err = glGetError();
	if (err != GL_NO_ERROR) {
		std::stringstream ss;
		ss << "OpenGL error(s) during frame:\n";
		while (err != GL_NO_ERROR) {
			ss << glerr_to_string(err) << '\n';
			err = glGetError();
		}
		Error("%s", ss.str().c_str());
	}
#endif

	GetWindow()->SwapBuffers();
	return true;
}

bool RendererGL2::SetRenderState(RenderState *rs)
{
	if (m_activeRenderState != rs) {
		static_cast<GL2::RenderState*>(rs)->Apply();
		m_activeRenderState = rs;
	}
	return true;
}

bool RendererGL2::SetRenderTarget(RenderTarget *rt)
{
	PROFILE_SCOPED()
	if (m_activeRenderTarget && rt != m_activeRenderTarget)
		m_activeRenderTarget->Unbind();
	if (rt)
		static_cast<GL2::RenderTarget*>(rt)->Bind();

	m_activeRenderTarget = static_cast<GL2::RenderTarget*>(rt);

	return true;
}

bool RendererGL2::ClearScreen()
{
	m_activeRenderState = nullptr;
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return true;
}

bool RendererGL2::ClearDepthBuffer()
{
	m_activeRenderState = nullptr;
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	return true;
}

bool RendererGL2::SetClearColor(const Color &c)
{
	glClearColor(
		static_cast<GLclampf>(c.r / 255.0f), 
		static_cast<GLclampf>(c.g / 255.0f), 
		static_cast<GLclampf>(c.b / 255.0f), 
		static_cast<GLclampf>(c.a / 255.0f));
	return true;
}

bool RendererGL2::SetViewport(int x, int y, int width, int height)
{
	assert(!m_viewportStack.empty());
	ViewportState& currentViewport = m_viewportStack.top();
	currentViewport.x = x;
	currentViewport.y = y;
	currentViewport.w = width;
	currentViewport.h = height;
	glViewport(x, y, width, height);
	return true;
}

bool RendererGL2::SetTransform(const matrix4x4d &m)
{
	PROFILE_SCOPED()
	matrix4x4f mf;
	matrix4x4dtof(m, mf);
	return SetTransform(mf);
}

bool RendererGL2::SetTransform(const matrix4x4f &m)
{
	PROFILE_SCOPED()
	//same as above
	m_modelViewStack.top() = m;
	SetMatrixMode(MatrixMode::MODELVIEW);
	LoadMatrix(&m[0]);
	return true;
}

bool RendererGL2::SetPerspectiveProjection(float fov, float aspect, float near, float far)
{
	PROFILE_SCOPED()

	// update values for log-z hack
	m_invLogZfarPlus1 = 1.0f / (log(far+1.0f)/log(2.0f));

	Graphics::SetFov(fov);

	float ymax = near * tan(fov * M_PI / 360.0);
	float ymin = -ymax;
	float xmin = ymin * aspect;
	float xmax = ymax * aspect;

	const matrix4x4f frustrumMat = matrix4x4f::FrustumMatrix(xmin, xmax, ymin, ymax, near, far);
	SetProjection(frustrumMat);
	return true;
}

bool RendererGL2::SetOrthographicProjection(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
	PROFILE_SCOPED()
	const matrix4x4f orthoMat = matrix4x4f::OrthoFrustum(xmin, xmax, ymin, ymax, zmin, zmax);
	SetProjection(orthoMat);
	return true;
}

bool RendererGL2::SetProjection(const matrix4x4f &m)
{
	PROFILE_SCOPED()
	//same as above
	m_projectionStack.top() = m;
	SetMatrixMode(MatrixMode::PROJECTION);
	LoadMatrix(&m[0]);
	return true;
}

bool RendererGL2::SetWireFrameMode(bool enabled)
{
	glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
	return true;
}

bool RendererGL2::SetLights(int numlights, const Light *lights)
{
	assert(!m_lightsStack.empty());
	LightsState &ls = m_lightsStack.top();
	if (numlights < 1) return false;

	// XXX move lighting out to shaders

	//glLight depends on the current transform, but we have always
	//relied on it being identity when setting lights.
	Graphics::Renderer::MatrixTicket ticket(this, MatrixMode::MODELVIEW);
	SetTransform(matrix4x4f::Identity());

	m_numLights = numlights;
	ls.numLights = 0;
	memcpy(ls.lights, lights, sizeof(Light) * std::min(MATERIAL_MAX_LIGHTS, numlights));

	for (int i=0; i < numlights; i++) {
		const Light &l = lights[i];
		// directional lights have w of 0
		const float pos[] = {
			l.GetPosition().x,
			l.GetPosition().y,
			l.GetPosition().z,
			l.GetType() == Light::LIGHT_DIRECTIONAL ? 0.f : 1.f
		};
		glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, l.GetDiffuse().ToColor4f());
		glLightfv(GL_LIGHT0+i, GL_SPECULAR, l.GetSpecular().ToColor4f());
		glEnable(GL_LIGHT0+i);

		if (l.GetType() == Light::LIGHT_DIRECTIONAL)
			ls.numLights++;

		assert(ls.numLights < 5);
	}

	return true;
}

bool RendererGL2::SetAmbientColor(const Color &c)
{
	m_ambient = c;
	return true;
}

bool RendererGL2::SetScissor(bool enabled, const vector2f &pos, const vector2f &size)
{
	assert(!m_scissorStack.empty());
	if (enabled) {
		glScissor(pos.x, pos.y, size.x, size.y);
		glEnable(GL_SCISSOR_TEST);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
	m_scissorStack.top().enable = enabled;
	m_scissorStack.top().position = pos;
	m_scissorStack.top().size = size;
	return true;
}

bool RendererGL2::DrawLines(int count, const vector3f *v, const Color *c, RenderState* state, LineType t)
{
	PROFILE_SCOPED()
	if (count < 2 || !v) return false;

	SetRenderState(state);

	vtxColorProg->Use();
	vtxColorProg->invLogZfarPlus1.Set(m_invLogZfarPlus1);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vector3f), v);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Color), c);
	glDrawArrays(t, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	return true;
}

bool RendererGL2::DrawLines(int count, const vector3f *v, const Color &c, RenderState *state, LineType t)
{
	PROFILE_SCOPED()
	if (count < 2 || !v) return false;

	SetRenderState(state);

	flatColorProg->Use();
	flatColorProg->diffuse.Set(c);
	flatColorProg->invLogZfarPlus1.Set(m_invLogZfarPlus1);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(vector3f), v);
	glDrawArrays(t, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);

	return true;
}

bool RendererGL2::DrawLines2D(int count, const vector2f *v, const Color &c, Graphics::RenderState* state, LineType t)
{
	if (count < 2 || !v) return false;

	SetRenderState(state);

	flatColorProg->Use();
	flatColorProg->diffuse.Set(c);
	flatColorProg->invLogZfarPlus1.Set(m_invLogZfarPlus1);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, sizeof(vector2f), v);
	glDrawArrays(t, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);

	return true;
}

bool RendererGL2::DrawPoints(int count, const vector3f *points, const Color *colors, Graphics::RenderState *state, float size)
{
	if (count < 1 || !points || !colors) return false;

	pointsColorProg->Use();
	pointsColorProg->invLogZfarPlus1.Set(m_invLogZfarPlus1);
	pointsColorProg->pointSize.Set(size);

	SetRenderState(state);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, points);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
	glDrawArrays(GL_POINTS, 0, count);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	return true;
}

bool RendererGL2::DrawTriangles(VertexArray *v, RenderState *rs, Material *m, PrimitiveType t)
{
	if (!v || v->position.size() < 3) return false;

	SetRenderState(rs);

	m->Apply();
	EnableClientStates(v);

	glDrawArrays(t, 0, v->GetNumVerts());

	m->Unapply();
	DisableClientStates();

	return true;
}

bool RendererGL2::DrawTriangles(int vertCount, VertexArray *vertices, RenderState *state, Material *material, PrimitiveType type)
{
	if(!vertices || vertices->position.size() < 3 || static_cast<unsigned>(vertCount) > vertices->GetNumVerts()) return false;

	SetRenderState(state);

	material->Apply();
	EnableClientStates(vertices);

	glDrawArrays(type, 0, vertCount);

	material->Unapply();
	DisableClientStates();

	return true;
}

bool RendererGL2::DrawPointSprites(int count, const vector3f *positions, RenderState *rs, Material *material, float size)
{
	if (count < 1 || !material || !material->texture0) return false;

	VertexArray va(ATTRIB_POSITION | ATTRIB_UV0, count * 6);

	matrix4x4f rot(GetCurrentModelView());
	rot.ClearToRotOnly();
	rot = rot.InverseOf();

	const float sz = 0.5f*size;
	const vector3f rotv1 = rot * vector3f(sz, sz, 0.0f);
	const vector3f rotv2 = rot * vector3f(sz, -sz, 0.0f);
	const vector3f rotv3 = rot * vector3f(-sz, -sz, 0.0f);
	const vector3f rotv4 = rot * vector3f(-sz, sz, 0.0f);

	//do two-triangle quads. Could also do indexed surfaces.
	//GL2 renderer should use actual point sprites
	//(see history of Render.cpp for point code remnants)
	for (int i=0; i<count; i++) {
		const vector3f &pos = positions[i];

		va.Add(pos+rotv4, vector2f(0.f, 0.f)); //top left
		va.Add(pos+rotv3, vector2f(0.f, 1.f)); //bottom left
		va.Add(pos+rotv1, vector2f(1.f, 0.f)); //top right

		va.Add(pos+rotv1, vector2f(1.f, 0.f)); //top right
		va.Add(pos+rotv3, vector2f(0.f, 1.f)); //bottom left
		va.Add(pos+rotv2, vector2f(1.f, 1.f)); //bottom right
	}

	DrawTriangles(&va, rs, material);

	return true;
}

bool RendererGL2::DrawBuffer(VertexBuffer* vb, RenderState* state, Material* mat, PrimitiveType pt)
{
	SetRenderState(state);
	mat->Apply();

	auto gvb = static_cast<GL2::VertexBuffer*>(vb);

	glBindBuffer(GL_ARRAY_BUFFER, gvb->GetBuffer());

	gvb->SetAttribPointers();
	EnableClientStates(gvb);

	glDrawArrays(pt, 0, gvb->GetVertexCount());

	gvb->UnsetAttribPointers();
	DisableClientStates();

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

bool RendererGL2::DrawBufferIndexed(VertexBuffer *vb, IndexBuffer *ib, RenderState *state, 
	Material *mat, PrimitiveType pt, unsigned start_index, unsigned index_count)
{
	SetRenderState(state);
	mat->Apply();

	auto gvb = static_cast<GL2::VertexBuffer*>(vb);
	auto gib = static_cast<GL2::IndexBuffer*>(ib);

	glBindBuffer(GL_ARRAY_BUFFER, gvb->GetBuffer());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gib->GetBuffer());

	gvb->SetAttribPointers();
	EnableClientStates(gvb);

	unsigned ic = index_count == 0? ib->GetIndexCount() : index_count;
	glDrawElements(pt, ic, GL_UNSIGNED_INT, (void*)(start_index * sizeof(Uint32)));

	gvb->UnsetAttribPointers();
	DisableClientStates();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return true;
}

bool RendererGL2::DrawFullscreenQuad(Material *mat, RenderState *state, bool clear_rt)
{
	assert(mat);
	mat->Apply();
	bool result = DrawFullscreenQuad(state, clear_rt);
	mat->Unapply();
	return result;
}

bool RendererGL2::DrawFullscreenQuad(Texture *texture, RenderState *state, bool clear_rt)
{
	assert(texture);
	m_screenQuadMtrl->texture0 = texture;
	bool result = DrawFullscreenQuad(m_screenQuadMtrl.get(), state, clear_rt);
	m_screenQuadMtrl->texture0 = nullptr;
	return result;
}

bool RendererGL2::DrawFullscreenQuad(RenderState *state, bool clear_rt)
{
	state = state == nullptr ? m_screenQuadRS : state;
	SetRenderState(state);
	if(clear_rt) {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_screenQuadBufferId);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, static_cast<void*>(0));

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}


void RendererGL2::EnableClientStates(const VertexArray *v)
{
	PROFILE_SCOPED();

	if (!v) return;
	assert(v->position.size() > 0); //would be strange

	// XXX could be 3D or 2D
	m_clientStates.push_back(GL_VERTEX_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *>(&v->position[0]));

	if (v->HasAttrib(ATTRIB_DIFFUSE)) {
		assert(! v->diffuse.empty());
		m_clientStates.push_back(GL_COLOR_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<const GLvoid *>(&v->diffuse[0]));
	}
	if (v->HasAttrib(ATTRIB_NORMAL)) {
		assert(! v->normal.empty());
		m_clientStates.push_back(GL_NORMAL_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, reinterpret_cast<const GLvoid *>(&v->normal[0]));
	}
	if (v->HasAttrib(ATTRIB_UV0)) {
		assert(! v->uv0.empty());
		m_clientStates.push_back(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, reinterpret_cast<const GLvoid *>(&v->uv0[0]));
	}
}

void RendererGL2::EnableClientStates(const VertexBuffer *vb)
{
	if (!vb) return;
	const auto& vbd = vb->GetDesc();

	for (Uint32 i = 0; i < MAX_ATTRIBS; i++) {
		switch (vbd.attrib[i].semantic) {
		case ATTRIB_POSITION:
			m_clientStates.push_back(GL_VERTEX_ARRAY);
			break;
		case ATTRIB_DIFFUSE:
			m_clientStates.push_back(GL_COLOR_ARRAY);
			break;
		case ATTRIB_NORMAL:
			m_clientStates.push_back(GL_NORMAL_ARRAY);
			break;
		case ATTRIB_UV0:
			m_clientStates.push_back(GL_TEXTURE_COORD_ARRAY);
			break;
		default:
			break;
		}
	}

	for (auto it : m_clientStates)
		glEnableClientState(it);
}

void RendererGL2::DisableClientStates()
{
	PROFILE_SCOPED();

	for (std::vector<GLenum>::const_iterator i = m_clientStates.begin(); i != m_clientStates.end(); ++i)
		glDisableClientState(*i);
	m_clientStates.clear();
}

Material *RendererGL2::CreateMaterial(const MaterialDescriptor &d)
{
	PROFILE_SCOPED()
	MaterialDescriptor desc = d;

	GL2::Material *mat = 0;
	GL2::Program *p = 0;

	if (desc.lighting) {
		desc.dirLights = m_lightsStack.top().numLights;
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
	case EFFECT_RADIAL_BLUR:
		mat = new Effects::RadialBlurMaterial();
		break;
	case EFFECT_BLOOM_COMPOSITOR:
		mat = new GL2::BloomCompositorMaterial();
		break;
	case EFFECT_SHIELD:
		mat = new GL2::ShieldMaterial();
		break;
	case EFFECT_SKYBOX:
		mat = new GL2::SkyboxMaterial();
		break;
	case EFFECT_SPHEREIMPOSTOR:
		mat = new GL2::SphereImpostorMaterial();
		break;

	case EFFECT_THRUSTERTRAILS_DEPTH:
		mat = new Effects::ThrusterTrailsDepthMaterial();
		break;

	case EFFECT_THRUSTERTRAILS:
		mat = new Effects::ThrusterTrailsMaterial();
		break;

	case EFFECT_SECTORVIEW_ICON:
		mat = new Effects::SectorViewIconMaterial();
		break;

	case EFFECT_TRANSIT_TUNNEL:
		mat = new Effects::TransitEffectMaterial();
		break;

	case EFFECT_TRANSIT_COMPOSITION:
		mat = new Effects::TransitCompositionMaterial();
		break;

	default:
		if (desc.lighting)
			mat = new GL2::LitMultiMaterial();
		else
			mat = new GL2::MultiMaterial();
	}

	mat->m_renderer = this;
	mat->m_descriptor = desc;

	p = GetOrCreateProgram(mat); // XXX throws ShaderException on compile/link failure

	mat->SetProgram(p);
	return mat;
}

bool RendererGL2::ReloadShaders()
{
	Output("Reloading " SIZET_FMT " programs...\n", m_programs.size());
	for (ProgramIterator it = m_programs.begin(); it != m_programs.end(); ++it) {
		it->second->Reload();
	}
	Output("Done.\n");

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

Texture *RendererGL2::CreateTexture(const TextureDescriptor &descriptor)
{
	return new TextureGL(descriptor, m_useCompressedTextures);
}

RenderState *RendererGL2::CreateRenderState(const RenderStateDesc &desc)
{
	const uint32_t hash = lookup3_hashlittle(&desc, sizeof(RenderStateDesc), 0);
	auto it = m_renderStates.find(hash);
	if (it != m_renderStates.end())
		return it->second;
	else {
		auto *rs = new GL2::RenderState(desc);
		m_renderStates[hash] = rs;
		return rs;
	}
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

VertexBuffer *RendererGL2::CreateVertexBuffer(const VertexBufferDesc &desc)
{
	return new GL2::VertexBuffer(desc);
}

IndexBuffer *RendererGL2::CreateIndexBuffer(Uint32 size, BufferUsage usage)
{
	return new GL2::IndexBuffer(size, usage);
}

static void dump_opengl_value(std::ostream &out, const char *name, GLenum id, int num_elems)
{
	assert(num_elems > 0 && num_elems <= 4);
	assert(name);

	GLdouble e[4];
	glGetDoublev(id, e);

	GLenum err = glGetError();
	if (err == GL_NO_ERROR) {
		out << name << " = " << e[0];
		for (int i = 1; i < num_elems; ++i)
			out << ", " << e[i];
		out << "\n";
	} else {
		while (err != GL_NO_ERROR) {
			if (err == GL_INVALID_ENUM) { out << name << " -- not supported\n"; }
			else { out << name << " -- unexpected error (" << err << ") retrieving value\n"; }
			err = glGetError();
		}
	}
}

bool RendererGL2::PrintDebugInfo(std::ostream &out)
{
	out << "OpenGL version " << glGetString(GL_VERSION);
	out << ", running on " << glGetString(GL_VENDOR);
	out << " " << glGetString(GL_RENDERER) << "\n";

	out << "GLEW version " << glewGetString(GLEW_VERSION) << "\n";

	if (glewIsSupported("GL_VERSION_2_0"))
		out << "Shading language version: " <<  glGetString(GL_SHADING_LANGUAGE_VERSION_ARB) << "\n";

	out << "Available extensions:" << "\n";
	GLint numext = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numext);
	if (glewIsSupported("GL_VERSION_3_0")) {
		for (int i = 0; i < numext; ++i) {
			out << "  " << glGetStringi(GL_EXTENSIONS, i) << "\n";
		}
	}
	else {
		out << "  ";
		std::istringstream ext(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
		std::copy(
			std::istream_iterator<std::string>(ext),
			std::istream_iterator<std::string>(),
			std::ostream_iterator<std::string>(out, "\n  "));
	}

	out << "\nImplementation Limits:\n";

	// first, clear all OpenGL error flags
	while (glGetError() != GL_NO_ERROR) {}

#define DUMP_GL_VALUE(name) dump_opengl_value(out, #name, name, 1)
#define DUMP_GL_VALUE2(name) dump_opengl_value(out, #name, name, 2)

	DUMP_GL_VALUE(GL_MAX_3D_TEXTURE_SIZE);
	DUMP_GL_VALUE(GL_MAX_ATTRIB_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_CLIP_PLANES);
	DUMP_GL_VALUE(GL_MAX_COLOR_ATTACHMENTS_EXT);
	DUMP_GL_VALUE(GL_MAX_COLOR_MATRIX_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
	DUMP_GL_VALUE(GL_MAX_CUBE_MAP_TEXTURE_SIZE);
	DUMP_GL_VALUE(GL_MAX_DRAW_BUFFERS);
	DUMP_GL_VALUE(GL_MAX_ELEMENTS_INDICES);
	DUMP_GL_VALUE(GL_MAX_ELEMENTS_VERTICES);
	DUMP_GL_VALUE(GL_MAX_EVAL_ORDER);
	DUMP_GL_VALUE(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS);
	DUMP_GL_VALUE(GL_MAX_LIGHTS);
	DUMP_GL_VALUE(GL_MAX_LIST_NESTING);
	DUMP_GL_VALUE(GL_MAX_MODELVIEW_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_NAME_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_PIXEL_MAP_TABLE);
	DUMP_GL_VALUE(GL_MAX_PROJECTION_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_RENDERBUFFER_SIZE_EXT);
	DUMP_GL_VALUE(GL_MAX_SAMPLES_EXT);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_COORDS);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_IMAGE_UNITS);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_LOD_BIAS);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_SIZE);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_STACK_DEPTH);
	DUMP_GL_VALUE(GL_MAX_TEXTURE_UNITS);
	DUMP_GL_VALUE(GL_MAX_VARYING_FLOATS);
	DUMP_GL_VALUE(GL_MAX_VERTEX_ATTRIBS);
	DUMP_GL_VALUE(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
	DUMP_GL_VALUE(GL_MAX_VERTEX_UNIFORM_COMPONENTS);
	DUMP_GL_VALUE(GL_NUM_COMPRESSED_TEXTURE_FORMATS);
	DUMP_GL_VALUE(GL_SAMPLE_BUFFERS);
	DUMP_GL_VALUE(GL_SAMPLES);
	DUMP_GL_VALUE2(GL_ALIASED_LINE_WIDTH_RANGE);
	DUMP_GL_VALUE2(GL_ALIASED_POINT_SIZE_RANGE);
	DUMP_GL_VALUE2(GL_MAX_VIEWPORT_DIMS);
	DUMP_GL_VALUE2(GL_SMOOTH_LINE_WIDTH_RANGE);
	DUMP_GL_VALUE2(GL_SMOOTH_POINT_SIZE_RANGE);

#undef DUMP_GL_VALUE
#undef DUMP_GL_VALUE2

	return true;
}

void RendererGL2::SetMatrixMode(MatrixMode mm)
{
	PROFILE_SCOPED()
	if( mm != m_matrixMode ) {
		switch (mm) {
			case MatrixMode::MODELVIEW:
				glMatrixMode(GL_MODELVIEW);
				break;
			case MatrixMode::PROJECTION:
				glMatrixMode(GL_PROJECTION);
				break;
		}
		m_matrixMode = mm;
	}
}

void RendererGL2::PushMatrix()
{
	PROFILE_SCOPED()

	glPushMatrix();
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.push(m_modelViewStack.top());
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.push(m_projectionStack.top());
			break;
	}
}

void RendererGL2::PopMatrix()
{
	PROFILE_SCOPED()
	glPopMatrix();
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.pop();
			assert(m_modelViewStack.size());
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.pop();
			assert(m_projectionStack.size());
			break;
	}
}

void RendererGL2::LoadIdentity()
{
	PROFILE_SCOPED()
	glLoadIdentity();
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top() = matrix4x4f::Identity();
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.top() = matrix4x4f::Identity();
			break;
	}
}

void RendererGL2::LoadMatrix(const matrix4x4f &m)
{
	PROFILE_SCOPED()
	glLoadMatrixf(&m[0]);
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top() = m;
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.top() = m;
			break;
	}
}

void RendererGL2::Translate( const float x, const float y, const float z )
{
	PROFILE_SCOPED()
	glTranslatef(x,y,z);
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top().Translate(x,y,z);
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.top().Translate(x,y,z);
			break;
	}
}

void RendererGL2::Scale( const float x, const float y, const float z )
{
	PROFILE_SCOPED()
	glScalef(x,y,z);
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top().Scale(x,y,z);
			break;
		case MatrixMode::PROJECTION:
			m_modelViewStack.top().Scale(x,y,z);
			break;
	}
}

}
