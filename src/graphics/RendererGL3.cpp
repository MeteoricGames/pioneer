// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "RendererGL3.h"
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
#include "gl3/ProgramGL3.h"
#include "gl3/Effect.h"
#include "gl3/EffectMaterial.h"
#include "gl3/RenderStateGL3.h"
#include "gl3/RenderTargetGL3.h"
#include "gl3/VertexBufferGL3.h"
#include "gl3/BuiltInShaders.h"
#include "gl3/Effect.h"
#include <stddef.h> //for offsetof
#include <ostream>
#include <sstream>
#include <iterator>

namespace Graphics { 

typedef std::vector<std::pair<MaterialDescriptor, GL3::Program*> >::const_iterator ProgramIterator;

// for material-less line and point drawing
GL3::EffectMaterial *vertexColorMaterial;
GL3::EffectMaterial *flatColorMaterial;
GL3::EffectMaterial *pointsColorMaterial;

RendererGL3::RendererGL3(WindowSDL *window, const Graphics::Settings &vs)
: Renderer(window, window->GetWidth(), window->GetHeight())
//the range is very large due to a "logarithmic z-buffer" trick used
//http://outerra.blogspot.com/2009/08/logarithmic-z-buffer.html
//http://www.gamedev.net/blog/73/entry-2006307-tip-of-the-day-logarithmic-zbuffer-artifacts-fix/
, m_minZNear(0.0001f)
, m_maxZFar(10000000.0f)
, m_defaultVAO(0)
, m_useCompressedTextures(false)
, m_invLogZfarPlus1(0.f)
, m_activeRenderTarget(nullptr)
, m_activeRenderState(nullptr)
, m_matrixMode(MatrixMode::MODELVIEW)
, m_activeEffect(nullptr)
{
	const bool useDXTnTextures = vs.useTextureCompression && glewIsSupported("GL_EXT_texture_compression_s3tc");
	m_useCompressedTextures = useDXTnTextures;

	// Initial states
    // - Default VAO required for core profile since VAO 0 has been removed
    glGenVertexArrays(1, &m_defaultVAO);
    glBindVertexArray(m_defaultVAO);
    //
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	SetMatrixMode(MatrixMode::MODELVIEW);
	m_modelViewStack.push(matrix4x4f::Identity());
	m_projectionStack.push(matrix4x4f::Identity());

	SetClearColor(Color(0.f));
	SetViewport(0, 0, m_width, m_height);
	memset(m_lights, 0, sizeof(LightSource)* MATERIAL_MAX_LIGHTS);

	if (vs.enableDebugMessages)
		GLDebug::Enable();

	// Define renderer shared uniform blocks
	GL3::Effect::DefineSharedUniformBlock("UBMaterial", sizeof(MaterialBlock));
	GL3::Effect::DefineSharedUniformBlock("UBLightSources", sizeof(LightSource) * MATERIAL_MAX_LIGHTS);
	
	// Flat color materials
	GL3::EffectDescriptorDirect fc_desc;
	fc_desc.uniforms.push_back("su_ModelViewProjectionMatrix");
	fc_desc.uniforms.push_back("invLogZfarPlus1");
	fc_desc.uniforms.push_back("material.diffuse");
	fc_desc.vertex_shader = GL3::Shaders::VertexColorVS;
	fc_desc.vertex_shader_debug_name = "VS->VertexColor(Flat)";
	fc_desc.fragment_shader = GL3::Shaders::VertexColorFS;
	fc_desc.fragment_shader_debug_name = "FS->VertexColor(Flat)";
	flatColorMaterial = new GL3::EffectMaterial(this, fc_desc);
	fc_desc.settings.push_back("VERTEX_COLOR");
	fc_desc.vertex_shader_debug_name = "VS->VertexColor(Vertex)";
	fc_desc.fragment_shader_debug_name = "FS->VertexColor(Vertex)";
	vertexColorMaterial = new GL3::EffectMaterial(this, fc_desc);
	fc_desc.settings.push_back("POINTS");
	fc_desc.uniforms.push_back("pointSize");
	fc_desc.vertex_shader_debug_name = "VS->VertexColor(Points)";
	fc_desc.fragment_shader_debug_name = "FS->VertexColor(Points)";
	pointsColorMaterial = new GL3::EffectMaterial(this, fc_desc);

	// Init fullscreen quad
	VertexBufferDesc vbd;
	vbd.attrib[0].semantic = ATTRIB_POSITION;
	vbd.attrib[0].format = ATTRIB_FORMAT_FLOAT4;
	vbd.numVertices = 6;
	vbd.usage = BUFFER_USAGE_STATIC;

	const float screenquad_vertices[] = {
		-1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
	};

	m_screenQuadVB.reset(new GL3::VertexBuffer(vbd));
	Uint8* verts = m_screenQuadVB->Map<Uint8>(BufferMapMode::BUFFER_MAP_WRITE);
	memcpy(verts, screenquad_vertices, sizeof(float) * 6 * 4);
	m_screenQuadVB->Unmap();
	
	RenderStateDesc sqrs;
	sqrs.blendMode = BlendMode::BLEND_SOLID;
	sqrs.depthTest = false;
	sqrs.depthWrite = false;
	m_screenQuadRS = CreateRenderState(sqrs);

	// TEMP: Deprecated
	//MaterialDescriptor sqmd;
	//sqmd.effect = EFFECT_TEXTURED_FULLSCREEN_QUAD;
	//m_screenQuadMtrl.reset(CreateMaterial(sqmd));

	// Init post processing
	m_postprocessing.reset(new PostProcessing(this));

	// Init general VAs
	m_linesVA.reset(new VertexArray(ATTRIB_POSITION, 1024));
	m_linesDiffuseVA.reset(new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE, 1024));
	m_pointsVA.reset(new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE, 1024));
}

RendererGL3::~RendererGL3()
{
	for (auto state : m_renderStates)
		delete state.second;
}

bool RendererGL3::GetNearFarRange(float &near, float &far) const
{
	near = m_minZNear;
	far = m_maxZFar;
	return true;
}

bool RendererGL3::BeginFrame()
{
	PROFILE_SCOPED()
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return true;
}

bool RendererGL3::BeginPostProcessing(RenderTarget* rt_device, PostProcessLayer layer)
{
	m_postprocessing->SetDeviceRT(rt_device);
	m_postprocessing->BeginFrame(layer);
	return true;
}

bool RendererGL3::PostProcessFrame(PostProcess* postprocess)
{	
	m_postprocessing->Run(postprocess);
	return true;
}

bool RendererGL3::EndPostProcessing()
{
	m_postprocessing->EndFrame();
	return true;
}

bool RendererGL3::EndFrame()
{
	return true;
}

static std::string glerr_to_string(GLenum err)
{
	switch (err)
	{
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	case GL_STACK_OVERFLOW: //deprecated in GL3
		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW: //deprecated in GL3
		return "GL_STACK_UNDERFLOW";
	case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
		return "GL_INVALID_FRAMEBUFFER_OPERATION";
	default:
		return stringf("Unknown error 0x0%0{x}", err);
	}
}

bool RendererGL3::SwapBuffers()
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
		//Error("%s", ss.str().c_str());
	}
#endif

	GetWindow()->SwapBuffers();
	return true;
}

bool RendererGL3::SetRenderState(RenderState *rs)
{
	m_customRenderState = true;
	if (m_activeRenderState != rs) {
		static_cast<GL3::RenderState*>(rs)->Apply();
		m_activeRenderState = rs;
	}
	return true;
}

bool RendererGL3::SetRenderTarget(RenderTarget *rt)
{
	PROFILE_SCOPED()
	if (m_activeRenderTarget && rt != m_activeRenderTarget)
		m_activeRenderTarget->Unbind();
	if (rt)
		static_cast<GL3::RenderTarget*>(rt)->Bind();

	m_activeRenderTarget = static_cast<GL3::RenderTarget*>(rt);

	return true;
}

bool RendererGL3::ClearScreen()
{
	m_activeRenderState = nullptr;
	glDepthMask(GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return true;
}

bool RendererGL3::ClearDepthBuffer()
{
	m_activeRenderState = nullptr;
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	return true;
}

bool RendererGL3::SetClearColor(const Color &c)
{
	glClearColor(
		static_cast<GLclampf>(c.r / 255.0f), 
		static_cast<GLclampf>(c.g / 255.0f), 
		static_cast<GLclampf>(c.b / 255.0f), 
		static_cast<GLclampf>(c.a / 255.0f));
	return true;
}

bool RendererGL3::SetViewport(int x, int y, int width, int height)
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

bool RendererGL3::SetTransform(const matrix4x4d &m)
{
	PROFILE_SCOPED()
	matrix4x4f mf;
	matrix4x4dtof(m, mf);
	return SetTransform(mf);
}

bool RendererGL3::SetTransform(const matrix4x4f &m)
{
	PROFILE_SCOPED()
	//same as above
	m_modelViewStack.top() = m;
	SetMatrixMode(MatrixMode::MODELVIEW);
	return true;
}

bool RendererGL3::SetPerspectiveProjection(float fov, float aspect, float near, float far)
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

bool RendererGL3::SetOrthographicProjection(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
	PROFILE_SCOPED()
	const matrix4x4f orthoMat = matrix4x4f::OrthoFrustum(xmin, xmax, ymin, ymax, zmin, zmax);
	SetProjection(orthoMat);
	return true;
}

bool RendererGL3::SetProjection(const matrix4x4f &m)
{
	PROFILE_SCOPED()
	m_projectionStack.top() = m;
	SetMatrixMode(MatrixMode::PROJECTION);
	return true;
}

bool RendererGL3::SetWireFrameMode(bool enabled)
{
	glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
	return true;
}

bool RendererGL3::SetLights(int numlights, const Light *lights)
{
	assert(!m_lightsStack.empty());
	LightsState &ls = m_lightsStack.top();
	if (numlights < 1 || !lights) {
		ls.numLights = 0;
		m_numLights = 0;
		return false;
	}

	Graphics::Renderer::MatrixTicket ticket(this, MatrixMode::MODELVIEW);
	SetTransform(matrix4x4f::Identity());

	m_numLights = numlights;
	ls.numLights = 0;

	assert(m_numLights <= 4);

	for(int i = 0; i < numlights; i++) {
		const Light &l = lights[i];
		const vector3f &pos = l.GetPosition();
		assert(l.GetType() == Light::LIGHT_DIRECTIONAL);
		m_lights[i].position = pos.NormalizedSafe();
		m_lights[i].position.w = 0.0f;
		m_lights[i].diffuse = l.GetDiffuse().ToVector4f();
		m_lights[i].specular = l.GetSpecular().ToVector3f();
		m_lights[i].radius = l.GetRadius();
		ls.numLights += 1;
	}
	memcpy(ls.lights, lights, sizeof(Light)* ls.numLights);

	return true;
}

bool RendererGL3::SetAmbientColor(const Color &c)
{
	m_ambient = c;
	return true;
}

bool RendererGL3::SetScissor(bool enabled, const vector2f &pos, const vector2f &size)
{
	assert(!m_scissorStack.empty());
	if (enabled) {
		glScissor(pos.x,pos.y,size.x,size.y);
		glEnable(GL_SCISSOR_TEST);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
	m_scissorStack.top().enable = enabled;
	m_scissorStack.top().position = pos;
	m_scissorStack.top().size = size;
	return true;
}

bool RendererGL3::BeginDrawVB(VertexBuffer *vb, Material *mat)
{
	assert(vb && (m_activeEffect || mat));
	if (mat && m_activeEffect != mat->GetEffect()) {
		m_activeEffect = mat->GetEffect();
	}
	assert(m_activeEffect);
	if (mat == nullptr) {
		m_activeEffect->Apply();
	} else {
		mat->Apply();
	}
	vb->Bind();
	vb->SetAttribPointers(m_activeEffect);
	return true;
}

bool RendererGL3::EndDrawVB(VertexBuffer *vb, Material *mat)
{
	assert(vb);
	vb->UnsetAttribPointers(m_activeEffect);
	vb->Unbind();
	if (mat == nullptr) {
		m_activeEffect->Unapply();
	} else {
		mat->Unapply();
	}
	return true;
}

bool RendererGL3::DrawLines(int count, const vector3f *v, const Color *c, RenderState* state, LineType t)
{
	PROFILE_SCOPED()
	assert(count >= 2 && v && c);

	m_linesDiffuseVA->Clear();
	for (int i = 0; i < count; ++i) {
		m_linesDiffuseVA->Add(v[i], c[i]);
	}

	CheckAndSetRenderState(state);

	m_linesDiffuseVA->UpdateInternalVB();
	GL3::VertexBuffer* vb = m_linesDiffuseVA->GetVB();
	BeginDrawVB(vb, vtxColorMaterial);
	glDrawArrays(t, 0, count);
	EndDrawVB(vb, vtxColorMaterial);

	return true;
}

bool RendererGL3::DrawLines(int count, VertexArray *va, const Color *c, RenderState* state, LineType t)
{
	PROFILE_SCOPED()
	assert(count > 1 && va && c);

	CheckAndSetRenderState(state);

	va->UpdateInternalVB();
	GL3::VertexBuffer* vb = va->GetVB();
	BeginDrawVB(vb, vtxColorMaterial);
	glDrawArrays(t, 0, count);
	EndDrawVB(vb, vtxColorMaterial);

	return true;
}

bool RendererGL3::DrawLinesBuffer(int count, VertexBuffer *vb, RenderState* state, LineType type)
{
	PROFILE_SCOPED()
	if(count < 2 || !vb) {
		return false;
	}
	
	CheckAndSetRenderState(state);
	BeginDrawVB(vb, vtxColorMaterial);
	glDrawArrays(type, 0, count);
	EndDrawVB(vb, vtxColorMaterial);

	return true;
}

bool RendererGL3::DrawLines(int count, const vector3f *v, const Color &c, RenderState *state, LineType t)
{
	PROFILE_SCOPED()
	assert(count >= 2);

	VertexArray va(ATTRIB_POSITION, count);
	va.Clear();
	for (int i = 0; i < count; ++i) {
		va.Add(v[i]);
	}

	return DrawLines(count, &va, c, state, t);
}

bool RendererGL3::DrawLines(int count, VertexArray *va, const Color &c, RenderState *state, LineType t)
{
	PROFILE_SCOPED()
	assert(count >= 2 && va);

	CheckAndSetRenderState(state);

	flatColorMaterial->diffuse = c;
	va->UpdateInternalVB();
	GL3::VertexBuffer* vb = va->GetVB();
	BeginDrawVB(vb, flatColorMaterial);
	glDrawArrays(t, 0, count);
	EndDrawVB(vb, flatColorMaterial);

	return true;
}

bool RendererGL3::DrawLines2D(int count, const vector2f *v, const Color &c, 
	Graphics::RenderState* state, LineType t)
{
	PROFILE_SCOPED()
	assert(count >= 2 && v);

	CheckAndSetRenderState(state);

	m_linesVA->Clear();
	for(int i = 0; i < count; ++i) {
		m_linesVA->Add(v[i]);
	}
	return DrawLines(count, m_linesVA.get(), c, state, t);
}

bool RendererGL3::DrawPoints(int count, const vector3f *points, const Color *colors, 
	Graphics::RenderState *state, float size)
{
	PROFILE_SCOPED()
	assert(count > 0 && points && colors);

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	CheckAndSetRenderState(state);

	m_pointsVA->Clear();
	for(int i = 0; i < count; ++i) {
		m_pointsVA->Add(points[i], colors[i]);
	}

	pointsColorMaterial->pointSize = size;
	m_pointsVA->UpdateInternalVB();
	GL3::VertexBuffer* vb = m_pointsVA->GetVB();
	BeginDrawVB(vb, pointsColorMaterial);
	glDrawArrays(GL_POINTS, 0, count);
	EndDrawVB(vb, pointsColorMaterial);

	return true;
}

bool RendererGL3::DrawTriangles(VertexArray *v, RenderState *rs, Material *mat, PrimitiveType t)
{
	assert(m_activeEffect || mat);
	assert(v && v->GetNumVerts() >= 3);
	CheckAndSetRenderState(rs);
	v->UpdateInternalVB();
	GL3::VertexBuffer* vb = v->GetVB();
	BeginDrawVB(vb, mat);
	glDrawArrays(t, 0, v->GetNumVerts());
	EndDrawVB(vb, mat);

	return true;
}

bool RendererGL3::DrawTriangles(int vertCount, VertexArray *vertices, RenderState *state, Material *material, 
	PrimitiveType type)
{
	assert(vertCount >= 2 && vertices);
	assert(m_activeEffect || material);

	CheckAndSetRenderState(state);
	vertices->UpdateInternalVB();
	GL3::VertexBuffer* vb = vertices->GetVB();
	BeginDrawVB(vb, material);
	glDrawArrays(type, 0, vertCount);
	EndDrawVB(vb, material);

	return true;
}

bool RendererGL3::DrawPointSprites(int count, const vector3f *positions, RenderState *rs, Material *material, float size)
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

bool RendererGL3::DrawBuffer(VertexBuffer* vb, RenderState* state, Material* mat, PrimitiveType pt)
{
	assert(vb && (mat || m_activeEffect));

	CheckAndSetRenderState(state);

	BeginDrawVB(vb, mat);
	glDrawArrays(pt, 0, vb->GetVertexCount());
	EndDrawVB(vb, mat);

	return true;
}

bool RendererGL3::DrawBufferIndexed(VertexBuffer *vb, IndexBuffer *ib, RenderState *state, 
	Material *mat, PrimitiveType pt, unsigned start_index, unsigned index_count)
{ 
	CheckAndSetRenderState(state);

	BeginDrawVB(vb, mat);
	ib->Bind();
	unsigned ic = index_count == 0? ib->GetIndexCount() : index_count;
	glDrawElements(pt, ic, GL_UNSIGNED_INT, (void*)(start_index * sizeof(Uint32)));
	ib->Unbind();
	EndDrawVB(vb, mat);

	return true;
}

bool RendererGL3::DrawFullscreenQuad(Material *mat, RenderState *state, bool clear_rt)
{
	assert(mat);
	state = state == nullptr? m_screenQuadRS : state;

	SetFullscreenRenderState(state);

	if(clear_rt) {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	BeginDrawVB(m_screenQuadVB.get(), mat);	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	EndDrawVB(m_screenQuadVB.get(), mat);

	return true;
}

bool RendererGL3::DrawFullscreenQuad(Texture *texture, RenderState *state, bool clear_rt)
{
	//assert(texture);
	// TEMP: Deprecated
	//m_screenQuadMtrl->texture0 = texture;
	//bool result = DrawFullscreenQuad(m_screenQuadMtrl.get(), state, clear_rt);
	//m_screenQuadMtrl->texture0 = nullptr;
	//return result;
	return false;
}

bool RendererGL3::DrawFullscreenQuad(RenderState *state, bool clear_rt)
{
	assert(m_activeEffect);
	state = state == nullptr? m_screenQuadRS : state;

	SetFullscreenRenderState(state);
	
	if(clear_rt) {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	BeginDrawVB(m_screenQuadVB.get(), nullptr);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	EndDrawVB(m_screenQuadVB.get(), nullptr);

	return true;
}


void RendererGL3::EnableClientStates(const VertexArray *v)
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

void RendererGL3::EnableClientStates(const VertexBuffer *vb)
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

void RendererGL3::DisableClientStates()
{
	PROFILE_SCOPED();

	for (std::vector<GLenum>::const_iterator i = m_clientStates.begin(); i != m_clientStates.end(); ++i)
		glDisableClientState(*i);
	m_clientStates.clear();
}

bool RendererGL3::ReloadShaders()
{
	/*Output("Reloading " SIZET_FMT " programs...\n", m_programs.size());
	for (ProgramIterator it = m_programs.begin(); it != m_programs.end(); ++it) {
		it->second->Reload();
	}
	Output("Done.\n");*/

	return true;
}

GL3::Program* RendererGL3::GetOrCreateProgram(GL3::Material *mat)
{
	return nullptr;
}

Material *RendererGL3::CreateMaterial(const MaterialDescriptor &descriptor)
{
	return nullptr;
}

Texture *RendererGL3::CreateTexture(const TextureDescriptor &descriptor)
{
	return new TextureGL(descriptor, m_useCompressedTextures);
}

RenderState *RendererGL3::CreateRenderState(const RenderStateDesc &desc)
{
	const uint32_t hash = lookup3_hashlittle(&desc, sizeof(RenderStateDesc), 0);
	auto it = m_renderStates.find(hash);
	if (it != m_renderStates.end())
		return it->second;
	else {
		auto *rs = new GL3::RenderState(desc);
		m_renderStates[hash] = rs;
		return rs;
	}
}

RenderTarget *RendererGL3::CreateRenderTarget(const RenderTargetDesc &desc)
{
	GL3::RenderTarget* rt = new GL3::RenderTarget(desc);
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

VertexBuffer *RendererGL3::CreateVertexBuffer(const VertexBufferDesc &desc)
{
	return new GL3::VertexBuffer(desc);
}

IndexBuffer *RendererGL3::CreateIndexBuffer(Uint32 size, BufferUsage usage)
{
	return new GL3::IndexBuffer(size, usage);
}

void RendererGL3::SetEffect(GL3::Effect* effect)
{
	if(m_activeEffect != effect) {
		m_activeEffect = effect;
	}
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

bool RendererGL3::PrintDebugInfo(std::ostream &out)
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

void RendererGL3::SetMatrixMode(MatrixMode mm)
{
	PROFILE_SCOPED()
	m_matrixMode = mm;
}

void RendererGL3::PushMatrix()
{
	PROFILE_SCOPED()

	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.push(m_modelViewStack.top());
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.push(m_projectionStack.top());
			break;
	}
}

void RendererGL3::PopMatrix()
{
	PROFILE_SCOPED()
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

void RendererGL3::LoadIdentity()
{
	PROFILE_SCOPED()
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top() = matrix4x4f::Identity();
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.top() = matrix4x4f::Identity();
			break;
	}
}

void RendererGL3::LoadMatrix(const matrix4x4f &m)
{
	PROFILE_SCOPED()
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top() = m;
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.top() = m;
			break;
	}
}

void RendererGL3::Translate( const float x, const float y, const float z )
{
	PROFILE_SCOPED()
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top().Translate(x,y,z);
			break;
		case MatrixMode::PROJECTION:
			m_projectionStack.top().Translate(x,y,z);
			break;
	}
}

void RendererGL3::Scale( const float x, const float y, const float z )
{
	PROFILE_SCOPED()
	switch(m_matrixMode) {
		case MatrixMode::MODELVIEW:
			m_modelViewStack.top().Scale(x,y,z);
			break;
		case MatrixMode::PROJECTION:
			m_modelViewStack.top().Scale(x,y,z);
			break;
	}
}

void RendererGL3::CheckAndSetRenderState(RenderState* rs)
{
	if (!m_customRenderState) {
		if (m_activeRenderState != rs) {
			static_cast<GL3::RenderState*>(rs)->Apply();
			m_activeRenderState = rs;
		}
	}
	else {
		m_customRenderState = false;
	}
}

void RendererGL3::SetFullscreenRenderState(RenderState* rs)
{
	if (m_activeRenderState != rs) {
		static_cast<GL3::RenderState*>(rs)->Apply();
		m_activeRenderState = rs;
	}
}

}
