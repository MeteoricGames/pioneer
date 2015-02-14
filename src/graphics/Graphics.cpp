// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Graphics.h"
#include "FileSystem.h"
#include "Material.h"
#include "RendererGL2.h"
#include "RendererGL3.h"
#include "OS.h"
#include "GraphicsHardware.h"
#include "gl3/BuiltInShaders.h"
#include "gl3/Effect.h"
#include "gl3/EffectMaterial.h"
#include <iostream>

namespace Graphics {

Hardware::ERendererType Hardware::RendererType = Hardware::ERT_GL2;
bool Hardware::context_CoreProfile = false;
bool Hardware::supports_FramebufferObjects = false;
bool Hardware::supports_sRGBFramebuffers = false;

static bool initted = false;
Material *vtxColorMaterial;
static int width, height;
static float g_fov = 85.f;
static float g_fovFactor = 1.f;

int GetScreenWidth()
{
	return width;
}

int GetScreenHeight()
{
	return height;
}

float GetFov()
{
	return g_fov;
}

void SetFov(float fov)
{
	g_fov = fov;
	g_fovFactor = 2 * tan(DEG2RAD(g_fov) / 2.f);
}

float GetFovFactor()
{
	return g_fovFactor;
}

Renderer* Init(Settings vs)
{
	assert(!initted);
	if (initted) return 0;

	// no mode set, find an ok one
	if ((vs.width <= 0) || (vs.height <= 0)) {
		const std::vector<VideoMode> modes = GetAvailableVideoModes();
		assert(!modes.empty());

		vs.width = modes.front().width;
		vs.height = modes.front().height;
	}

	WindowSDL *window = new WindowSDL(vs, "Paragon");
	width = window->GetWidth();
	height = window->GetHeight();

	glewExperimental = true;
	glewInit();

	int major, minor;
	if(GLEW_VERSION_3_2) {
		major = 3;
		minor = 2;
	} else if(GLEW_VERSION_3_1) {
		major = 3;
		minor = 1;
	} else {
		major = 2;
		minor = 1;
	}
	std::cout<<"Glew reports support for OpenGL "<<major<<"."<<minor<<std::endl;

	if(GLEW_ARB_compatibility) {
		std::cout<<"Running in compatibility mode"<<std::endl;
		Graphics::Hardware::context_CoreProfile = false;
	} else {
		std::cout<<"Running in core mode"<<std::endl;
		Graphics::Hardware::context_CoreProfile = true;
	}
	std::cout<<"Renderer Details"<<std::endl;
	std::cout<<"  Vendor: "<<glGetString(GL_VENDOR)<<std::endl;
	std::cout<<"  Version: "<<glGetString(GL_VERSION)<<std::endl;
	std::cout<<"  Renderer: "<<glGetString(GL_RENDERER)<<std::endl;

	Renderer *renderer = 0;

	bool legacy_mode = false;

	if (!glewIsSupported("GL_VERSION_3_2") && !glewIsSupported("GL_VERSION_3_1") ) {
		Warning("OpenGL Version 3.1 and 3.2 are not supported. Paragon will try to run in legacy OpenGL 2.1 mode.");
		if (!glewIsSupported("GL_VERSION_2_1")) {
			Error("OpenGL Version 2.1 is not supported. Paragon cannot run on your graphics card.");
		} else {
			legacy_mode = true;
		}
	}

	if(legacy_mode) {
		Hardware::RendererType = Hardware::ERendererType::ERT_GL2;
		if (!glewIsSupported("GL_ARB_vertex_buffer_object")) {
			Error("OpenGL extension ARB_vertex_buffer_object not supported. Paragon can not run on your graphics card.");
		}
		if (!glewIsSupported("GL_EXT_framebuffer_object") && !glewIsSupported("GL_ARB_framebuffer_object")) {
			Warning("OpenGL extension EXT/ARB_framebuffer_object not supported. \nParagon's post-processing disabled.");
			Hardware::supports_FramebufferObjects = false;
		} else {
			Hardware::supports_FramebufferObjects = true;
		}
		if (!glewIsSupported("GL_EXT_framebuffer_sRGB") && !glewIsSupported("GL_ARB_framebuffer_sRGB")) {
			Warning("Required OpenGL extension EXT/ARB_framebuffer_sRGB not supported. \nParagon's enhanced anti-aliasing (SMAA) is disabled.");
			Hardware::supports_sRGBFramebuffers = false;
		} else {
			Hardware::supports_sRGBFramebuffers = true;
		}
		renderer = new RendererGL2(window, vs);
	} else {
		Hardware::RendererType = Hardware::ERendererType::ERT_GL3;	// OpenGL 3.X renderer
		Hardware::supports_FramebufferObjects = true;				// Core since 3.0
		Hardware::supports_sRGBFramebuffers = true;					// Core since 3.0
		renderer = new RendererGL3(window, vs);
	}

	Output("Initialized %s\n", renderer->GetName());

	initted = true;

	if(Hardware::GL3()) {
		GL3::EffectDescriptorDirect vc_desc;
		vc_desc.settings.push_back("VERTEX_COLOR");
		vc_desc.vertex_shader_debug_name = "G->VertexColorVS(Vertex)";
		vc_desc.fragment_shader_debug_name = "G->FragmentColorFS(Vertex)";
		vc_desc.vertex_shader = GL3::Shaders::VertexColorVS;
		vc_desc.fragment_shader = GL3::Shaders::VertexColorFS;
		vc_desc.uniforms.push_back("su_ModelViewProjectionMatrix");
		vc_desc.uniforms.push_back("invLogZfarPlus1");
		GL3::EffectMaterial* em = new GL3::EffectMaterial(renderer, vc_desc);
		vtxColorMaterial = em;
	} else {
		MaterialDescriptor desc;
		desc.vertexColors = true;
		vtxColorMaterial = renderer->CreateMaterial(desc);
		vtxColorMaterial->IncRefCount();
	}

	return renderer;
}

void Uninit()
{
	delete vtxColorMaterial;
}

static bool operator==(const VideoMode &a, const VideoMode &b) {
	return a.width==b.width && a.height==b.height;
}

std::vector<VideoMode> GetAvailableVideoModes()
{
	std::vector<VideoMode> modes;

	const int num_displays = SDL_GetNumVideoDisplays();
	for(int display_index = 0; display_index < num_displays; display_index++)
	{
		const int num_modes = SDL_GetNumDisplayModes(display_index);

		SDL_Rect display_bounds;
		SDL_GetDisplayBounds(display_index, &display_bounds);

		for (int display_mode = 0; display_mode < num_modes; display_mode++)
		{
			SDL_DisplayMode mode;
			SDL_GetDisplayMode(display_index, display_mode, &mode);
			// insert only if unique resolution
			if( modes.end()==std::find(modes.begin(), modes.end(), VideoMode(mode.w, mode.h)) ) {
				modes.push_back(VideoMode(mode.w, mode.h));
			}
		}
	}
	if( num_displays==0 ) {
		modes.push_back(VideoMode(800, 600));
	}
	return modes;
}

}
