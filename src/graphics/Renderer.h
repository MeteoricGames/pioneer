// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _RENDERER_H
#define _RENDERER_H

#include "WindowSDL.h"
#include "libs.h"
#include "graphics/Types.h"
#include "graphics/Light.h"
#include <map>
#include <stack>
#include <memory>
#include "graphics/GraphicsHardware.h"

#define MATERIAL_MAX_LIGHTS 4

namespace Graphics {

/*
 * Renderer base class. A Renderer draws points, lines, triangles.
 * It is also used to create render states, materials and vertex/index buffers.
 */

class Light;
class Material;
class MaterialDescriptor;
class RenderState;
class RenderTarget;
class Texture;
class TextureDescriptor;
class VertexArray;
class PostProcessing;
class PostProcess;
class VertexBuffer;
class IndexBuffer;
struct VertexBufferDesc;
struct RenderStateDesc;
struct RenderTargetDesc;

namespace GL3 { class Effect; }

enum class MatrixMode {
	MODELVIEW,
	PROJECTION
};

enum class CullMode {
	FRONT,
	BACK,
	FRONT_AND_BACK,
};

// Renderer base, functions return false if
// failed/unsupported
class Renderer
{
public:
	Renderer(WindowSDL *win, int width, int height);
	virtual ~Renderer();

	virtual const char* GetName() const = 0;

	WindowSDL *GetWindow() const { return m_window.get(); }
	PostProcessing* GetPostProcessing() const { return m_postprocessing.get(); }
	float GetDisplayAspect() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

	//get supported minimum for z near and maximum for z far values
	virtual bool GetNearFarRange(float &near, float &far) const = 0;

	virtual bool BeginFrame() = 0;
	virtual bool EndFrame() = 0;
	virtual bool BeginPostProcessing(RenderTarget* rt_device = nullptr) = 0;
	virtual bool PostProcessFrame(PostProcess* postprocess = nullptr) = 0;
	virtual bool EndPostProcessing() = 0;
	//traditionally gui happens between endframe and swapbuffers
	virtual bool SwapBuffers() = 0;

	//set 0 to render to screen
	virtual bool SetRenderTarget(RenderTarget*) { return false; }
	virtual RenderTarget* GetActiveRenderTarget() const { return nullptr; }

	//clear color and depth buffer
	virtual bool ClearScreen() { return false; }
	//clear depth buffer
	virtual bool ClearDepthBuffer() { return false; }
	virtual bool SetClearColor(const Color &c) { return false; }

	virtual bool SetViewport(int x, int y, int width, int height) { return false; }

	//set the model view matrix
	virtual bool SetTransform(const matrix4x4d &m) { return false; }
	virtual bool SetTransform(const matrix4x4f &m) { return false; }
	//set projection matrix
	virtual bool SetPerspectiveProjection(float fov, float aspect, float near, float far) { return false; }
	virtual bool SetOrthographicProjection(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax) { return false; }
	virtual bool SetProjection(const matrix4x4f &m) { return false; }

	virtual bool SetRenderState(RenderState*) { return false; }

	virtual bool SetWireFrameMode(bool enabled) { return false; }

	virtual bool SetLights(int numlights, const Light *l) { return false; }
	virtual int GetNumDirLights() const { return m_lightsStack.top().numLights; }
	virtual LightSource* GetLightSources() const { return const_cast<LightSource*>(m_lights); }
	virtual bool SetAmbientColor(const Color &c) { return false; }
	const Color &GetAmbientColor() const { return m_ambient; }

	virtual bool SetScissor(bool enabled, const vector2f &pos = vector2f(0.0f), const vector2f &size = vector2f(0.0f)) { return false; }

	//drawing functions
	virtual bool BeginDrawVB(VertexBuffer*, Material*) { return false; }
	virtual bool EndDrawVB(VertexBuffer*, Material*) { return false; }
	//2d drawing is generally understood to be for gui use (unlit, ortho projection)
	//per-vertex colour lines
	virtual bool DrawLines(int vertCount, const vector3f *vertices, const Color *colors, RenderState*, LineType type=LINE_SINGLE) { return false; }
	virtual bool DrawLines(int vertCount, VertexArray *va, const Color *colors, RenderState*, LineType type = LINE_SINGLE) { return false; }
	virtual bool DrawLinesBuffer(int vertCount, VertexBuffer *vb, RenderState*, LineType type = LINE_SINGLE) { return false; }
	//flat colour lines
	virtual bool DrawLines(int vertCount, const vector3f *vertices, const Color &color, RenderState*, LineType type=LINE_SINGLE) { return false; }
	virtual bool DrawLines(int vertCount, VertexArray *va, const Color &color, RenderState*, LineType type = LINE_SINGLE) { return false; }
	virtual bool DrawLines2D(int vertCount, const vector2f *vertices, const Color &color, RenderState*, LineType type=LINE_SINGLE) { return false; }
	virtual bool DrawPoints(int count, const vector3f *points, const Color *colors, RenderState*, float pointSize=1.f) { return false; }
	//unindexed triangle draw
	virtual bool DrawTriangles(VertexArray *vertices, RenderState *state, Material *material, PrimitiveType type=TRIANGLES)  { return false; }
	virtual bool DrawTriangles(int vertCount, VertexArray *vertices, RenderState *state, Material *material, PrimitiveType type=TRIANGLES)  { return false; }
	//high amount of textured quads for particles etc
	virtual bool DrawPointSprites(int count, const vector3f *positions, RenderState *rs, Material *material, float size) { return false; }
	//complex unchanging geometry that is worthwhile to store in VBOs etc.
	virtual bool DrawBuffer(VertexBuffer*, RenderState*, Material*, PrimitiveType type=TRIANGLES) { return false; }
	virtual bool DrawBufferIndexed(VertexBuffer*, IndexBuffer*, RenderState*, Material*, PrimitiveType=TRIANGLES, unsigned start_index = 0, unsigned index_count = 0) { return false; }
	virtual bool DrawFullscreenQuad(Material*, RenderState* state = nullptr, bool clear_rt = true) { return false; }
	virtual bool DrawFullscreenQuad(Texture*, RenderState* state = nullptr, bool clear_rt = true) { return false; }
	virtual bool DrawFullscreenQuad(RenderState *state, bool clear_rt) { return false; }

	//creates a unique material based on the descriptor. It will not be deleted automatically.
	virtual Material *CreateMaterial(const MaterialDescriptor &descriptor) = 0;
	virtual Texture *CreateTexture(const TextureDescriptor &descriptor) = 0;
	virtual RenderState *CreateRenderState(const RenderStateDesc &) = 0;
	//returns 0 if unsupported
	virtual RenderTarget *CreateRenderTarget(const RenderTargetDesc &) { return 0; }
	virtual VertexBuffer *CreateVertexBuffer(const VertexBufferDesc&) = 0;
	virtual IndexBuffer *CreateIndexBuffer(Uint32 size, BufferUsage) = 0;

	virtual void SetEffect(GL3::Effect* effect) { return; };

	Texture *GetCachedTexture(const std::string &type, const std::string &name);
	void AddCachedTexture(const std::string &type, const std::string &name, Texture *texture);
	void RemoveCachedTexture(const std::string &type, const std::string &name);
	void RemoveAllCachedTextures();

	// output human-readable debug info to the given stream
	virtual bool PrintDebugInfo(std::ostream &out) { return false; }

	virtual bool ReloadShaders() { return false; }

	// our own matrix stack
	// XXX state must die
	virtual const matrix4x4f& GetCurrentModelView() const  = 0;
	virtual const matrix4x4f& GetCurrentProjection() const  = 0;

	// XXX all quite GL specific. state must die!
	virtual void SetMatrixMode(MatrixMode mm) = 0;
	virtual void PushMatrix() = 0;
	virtual void PopMatrix() = 0;
	virtual void LoadIdentity() = 0;
	virtual void LoadMatrix(const matrix4x4f &m) = 0;
	virtual void Translate( const float x, const float y, const float z ) = 0;
	virtual void Scale( const float x, const float y, const float z ) = 0;

	virtual float GetInvLogZfarPlus1() const { return m_invLogZfarPlus1; }

	// take a ticket representing a single state matrix. when the ticket is
	// deleted, the previous matrix state is restored
	// XXX state must die
	class MatrixTicket {
	public:
		MatrixTicket(Renderer *r, MatrixMode m) : m_renderer(r), m_matrixMode(m) {
			m_renderer->SetMatrixMode(m_matrixMode);
			m_renderer->PushMatrix();
		}
		virtual ~MatrixTicket() {
			m_renderer->SetMatrixMode(m_matrixMode);
			m_renderer->PopMatrix();
		}
	private:
		MatrixTicket(const MatrixTicket&);
		MatrixTicket &operator=(const MatrixTicket&);
		Renderer *m_renderer;
		MatrixMode m_matrixMode;
	};

	// Standard parameters
	matrix4x4f CalcModelViewProjectionMatrix() const {
		return GetCurrentProjection() * GetCurrentModelView();
	}

	matrix3x3f CalcNormalMatrix() const {
		return GetCurrentModelView().To3x3Matrix().InverseTranspose();
	}

	matrix4x4f CalcModelViewMatrixInverse() const {
		return GetCurrentModelView().InverseOf();
	}

	matrix4x4f CalcProjectionMatrixInverse() const {
		return GetCurrentProjection().InverseOf();
	}

	matrix4x4f CalcModelViewProjectionMatrixInverse() const {
		return CalcModelViewProjectionMatrix().InverseOf();
	}

	matrix4x4f CalcModelViewMatrixTranspose() const {
		return GetCurrentModelView().Transpose();
	}

	matrix4x4f CalcProjectionMatrixTranspose() const {
		return GetCurrentProjection().Transpose();
	}

	matrix4x4f CalcModelViewProjectionMatrixTranspose() const {
		return CalcModelViewProjectionMatrix().Transpose();
	}

	matrix4x4f CalcModelViewMatrixInverseTranspose() const {
		return GetCurrentModelView().InverseTranspose();
	}

	matrix4x4f CalcProjectionMatrixInverseTranspose() const {
		return GetCurrentProjection().InverseTranspose();
	}

	matrix4x4f CalcModelViewProjectionMatrixInverseTranspose() const {
		return CalcModelViewProjectionMatrix().InverseTranspose();
	}

	matrix4x4f CalcViewMatrix() const {
		return m_currentViewTransform;
	}

	matrix4x4f CalcViewMatrixInverse() const {
		return m_currentViewTransform.InverseOf();
	}

	// OpenGL State Management

	// Line Width
	virtual void SetLineWidth(float width);
	class LineWidthTicket {
	public:
		LineWidthTicket(Renderer* r, float width = 1.0f) : m_renderer(r) {
			m_lineWidth = m_renderer->m_lineWidth;
			m_renderer->SetLineWidth(width);
		}
		virtual ~LineWidthTicket() {
			m_renderer->SetLineWidth(m_lineWidth);
		}
	private:
		LineWidthTicket(const LineWidthTicket&);
		LineWidthTicket& operator=(const LineWidthTicket&);
		float m_lineWidth;
		Renderer* m_renderer;
	};

	// Lights State
protected:
	struct LightsState {
		LightsState() : numLights(0) { }
		virtual ~LightsState() {}
		int numLights;
		Light lights [MATERIAL_MAX_LIGHTS];
	};
public:
	virtual void PushLightsState(int num_lights = 0, const Light* lights = nullptr);
	virtual void PopLightsState();
	class LightsTicket {
	public:
		LightsTicket(Renderer* r, int num_lights = 0, const Light* lights = nullptr) : m_renderer(r) {
			m_renderer->PushLightsState(num_lights, lights);
		}
		virtual ~LightsTicket() {
			m_renderer->PopLightsState();
		}
	private:
		LightsTicket(const LightsTicket&);
		LightsTicket& operator=(const LightsTicket&);
		Renderer *m_renderer;
	};

	// Viewport State
protected:
	struct ViewportState {
		ViewportState() : x(0), y(0), w(0), h(0) {}
		Sint32 x, y, w, h;
	};
public:
	virtual void PushViewportState(Sint32 x, Sint32 y, Sint32 width, Sint32 height);
	virtual void PopViewportState();
	class ViewportTicket {
	public:
		ViewportTicket(Renderer *r, Sint32 x = 0, Sint32 y = 0, Sint32 width = 0, Sint32 height = 0) : m_renderer(r) {
			m_renderer->PushViewportState(x, y, width, height);
		}
		virtual ~ViewportTicket() {
			m_renderer->PopViewportState();
		}
	private:
		ViewportTicket(const ViewportTicket&);
		ViewportTicket& operator=(const ViewportTicket&);
		Renderer* m_renderer;
	};
	virtual void GetCurrentViewport(Sint32 *vp) const {
		const ViewportState &cur = m_viewportStack.top();
		vp[0] = cur.x; vp[1] = cur.y; vp[2] = cur.w; vp[3] = cur.h;
	}

	// Scissor State
protected:
	struct ScissorState {
		ScissorState() : enable(false), position(vector2f(0,0)), size(vector2f(0,0)) {}
		bool enable;
		vector2f position;
		vector2f size;
	};
public:
	virtual void PushScissorState(bool enable_scissor = false, const vector2f &position = vector2f(0,0), 
		const vector2f &size = vector2f(0,0));
	virtual void PopScissorState();
	class ScissorTicket {
	public:
		ScissorTicket(Renderer* r, bool enable_scissor = false, const vector2f &position = vector2f(0, 0),
			const vector2f &size = vector2f(0, 0)) : m_renderer(r) {
			m_renderer->PushScissorState(enable_scissor, position, size);
		}
		virtual ~ScissorTicket() {
			m_renderer->PopScissorState();
		}
	private:
		ScissorTicket(const ScissorTicket&);
		ScissorTicket& operator=(const ScissorTicket&);
		Renderer* m_renderer;
	};

public:
	// Used for effects that require a separate view matrix, not for anything else!
	void SetCurrentViewTransform(const matrix4x4d& view_transform) { matrix4x4dtof(view_transform, m_currentViewTransform); }
	const matrix4x4f& GetCurrentViewTransform() const { return m_currentViewTransform; }

protected:
	int m_width;
	int m_height;
	Color m_ambient;
	std::unique_ptr<PostProcessing> m_postprocessing;
	LightSource m_lights [MATERIAL_MAX_LIGHTS]; // Used for shader blocks
	
	// OpenGL state management
	float m_lineWidth;
	std::stack<LightsState> m_lightsStack;		// Lighting
	std::stack<ViewportState> m_viewportStack;	// Viewport
	std::stack<ScissorState> m_scissorStack;	// Scissor

	matrix4x4f m_currentViewTransform;			// View transform for current body
												// This is used for effects that require a separate view matrix
	float m_invLogZfarPlus1;

private:
	typedef std::pair<std::string,std::string> TextureCacheKey;
	typedef std::map<TextureCacheKey,RefCountedPtr<Texture>*> TextureCacheMap;
	TextureCacheMap m_textures;

	std::unique_ptr<WindowSDL> m_window;
};

}

#endif
