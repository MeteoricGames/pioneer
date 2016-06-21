// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "libs.h"
#include "RefCounted.h"
#include "graphics/gl3/UniformGL3.h"
#include "graphics/gl3/AttributeGL3.h"
#include "utils.h"

namespace Graphics {
	class Renderer; 
	namespace GL3 {

class EffectProgram;
class EffectShader;
enum EffectShaderType : unsigned int;

enum EffectGLSLVersion
{
	ESV_GLSL_110 = 0,	// OpenGL 2.0
	ESV_GLSL_120,		// OpenGL 2.1
	ESV_GLSL_140,		// OpenGL 3.1
	ESV_GLSL_150,		// OpenGL 3.2
	ESV_GLSL_GL3,		// Default supported version: 1.4 on Windows/Linux, 1.5 on OSX.
};

static const char* GLSLVersions[] = {
	"#version 110\n", 
	"#version 120\n", 
	"#version 140\n", 
	"#version 150\n",
#if defined(_WIN32) || defined(__linux) || defined(__unix)
	"#version 140\n"
#elif defined(__APPLE__)
	"#version 150\n"
#endif
};

// Describes loading of vertex/fragment shaders from files with customizable version, settings, and uniforms list.
struct EffectDescriptor
{
	EffectDescriptor() : version(ESV_GLSL_GL3), strict(false) { }

	// Version of GLSL used to compile
	EffectGLSLVersion version;
	// If true, more error checks are performed like error on uniform not found. Don't use for ubershaders!
	bool strict;
	// +"#define {setting}" first thing added to effect (fragment/vertex)
	std::vector<std::string> settings;
	// Vertex shader file
	std::string vertex_shader;
	// Fragment shader file
	std::string fragment_shader;
	// Uniforms (names)
	std::vector<std::string> uniforms;
};

// Describes loading of vertex/fragment shaders directly by string.
struct EffectDescriptorDirect 
{
	EffectDescriptorDirect() : version(ESV_GLSL_GL3), strict(false) { }

	// Version of GLSL used to compile
	EffectGLSLVersion version;
	// If true, more error checks are performed like error on uniform not found. Don't use for ubershaders!
	bool strict;
	// +"#define {setting}" first thing added to effect (fragment/vertex)
	std::vector<std::string> settings;
	// Vertex shader debugging name
	std::string vertex_shader_debug_name;
	// Vertex shader code
	std::string vertex_shader;
	// Fragment shader debugging name
	std::string fragment_shader_debug_name;
	// Fragment shader code
	std::string fragment_shader;
	// Uniforms (names)
	std::vector<std::string> uniforms;
};

struct EffectFileCache
{
	char* data;
	unsigned int length;
};

/*
 * Effect class supports defining standard parameters.
 * When Apply() method is called Effect class automatically recalculates only necessary standard params and sets them
 * to their respective uniforms. The params that get updated are determined via the uniforms value in effect descriptor.
 * To define a new standard parameter add its name to this list then add its calculation in Effect::UpdateStandardParameter()
 * This method MUST ONLY be used for legacy effects. Newer effects should calculate and set uniforms in code for performance
 * reasons.
 */
static const std::vector<std::string> StandardParameters = {
// 0
"su_ModelViewMatrix",
// 1
"su_ProjectionMatrix",
// 2
"su_ModelViewProjectionMatrix",
// 3
"su_NormalMatrix",
// 4
"su_ModelViewMatrixInverse",
// 5
"su_ProjectionMatrixInverse",
// 6
"su_ModelViewProjectionMatrixInverse",
// 7
"su_ModelViewMatrixTranspose",
// 8
"su_ProjectionMatrixTranspose",
// 9
"su_ModelViewProjectionTranspose",
// 10
"su_ModelViewMatrixInverseTranspose",
// 11
"su_ProjectionMatrixInverseTranspose",
// 12
"su_ModelViewProjectionMatrixInverseTranspose",
// 13
"su_ViewMatrix",
// 14
"su_ViewMatrixInverse",
// 15
"su_ViewportSize",
// 16
"su_GameTime",
};

enum EEffectAttributes
{
	EEA_POSITION = 0,
	EEA_NORMAL,
	EEA_TEXCOORDS0,
	EEA_DIFFUSE,

	EEA_TOTAL,
};

static const std::vector<std::string> EffectAttributes = {
	"a_Vertex",
	"a_Normal",
	"a_MultiTexCoord0",
	"a_Color",
};

static const int EffectAttributesSizes [] = { 4, 4, 2, 4 };

class Effect : public RefCounted
{
public:
	struct UniformBlock
	{
		std::string name;
		int binding_point;
		GLuint ubuffer;
		size_t data_size;
	};

public:
	Effect(Renderer* renderer, const EffectDescriptor& effect_desc);
	Effect(Renderer* renderer, const EffectDescriptorDirect& effect_desc);
	virtual ~Effect();

	// Effect can be applied anytime for updating parameters before rendering. If it's already
	// applied the function does nothing. (so it can be called multiple times)
	virtual void SetProgram();
	virtual void UnsetProgram();
	virtual void Apply();
	virtual void Unapply();
	virtual void ChangeSettings(const std::vector<std::string>& new_settings);

	Attribute& GetAttribute(EEffectAttributes attrib) const;
	unsigned int GetProgramID() const;
	unsigned int GetUniformID(const char* name) const;
	Uniform& GetUniform(unsigned int id) const;
	bool IsApplied() const { return m_isApplied; }
	bool IsProgramUsed() const { return m_isProgramUsed; }
	const char* GetDebugString() const;

	// Uniform blocks
	int InitUniformBlock(int uniform_block);
	void WriteUniformBlock(int uniform_block, size_t data_size, void* data);

protected:
	friend class RendererGL3;

	void Init();
	void InitDirect();
	EffectShader* InitShader(EffectShaderType type, unsigned int hash);
	EffectProgram* InitProgram(unsigned int hash);
	void InitAttribs();
	void InitUniforms(unsigned int hash);

	unsigned int CalculateSettingsHash(const std::vector<std::string>& settings);
	unsigned int CalculateVSHash();
	unsigned int CalculateFSHash();
	unsigned int CalculateHash(const char* s, unsigned int seed = 0);
	EffectFileCache* LoadShader(const std::string& shader_file);
	void UpdateStandardParameters();

private:
	Effect(const Effect&);
	Effect& operator=(const Effect&);

	Renderer *m_renderer;
	EffectProgram* m_program;
	std::vector<Graphics::GL3::Uniform*> m_uniforms;
	std::map<std::string, unsigned int> m_uniformIndices;
	std::vector<Attribute*> m_attribs;
	int m_attribsStride;
	std::map<unsigned, unsigned>* m_stdParams; // standard param id, uniform index
	EffectShader* m_vertexShader;
	EffectShader* m_fragmentShader;
	std::string m_programDebugDesc;
	std::string m_vertexDebugDesc;
	std::string m_fragmentDebugDesc;
	EffectDescriptor m_effectDesc;
	EffectDescriptorDirect m_effectDescDirect;
	unsigned int m_curSettingsHash;
	unsigned int m_curVSHash;
	unsigned int m_curFSHash;
	bool m_isDirect;			// For directly shaders (no loading of files necessary)
	bool m_isApplied;			// Used to keep track of whether applied or not at the moment
	bool m_isProgramUsed;		// Used to keep track of whether program is used or not at the moment

public:
	static int DefineSharedUniformBlock(const char* name, size_t data_size);
	static int GetSharedUniformBlock(const char* name);
	static void StaticUpdate(float timeStep);

private: // Static

	static void StaticInit();
	static bool s_isInit;

	// This calls clears all cache for effect files, shaders, and programs
	// forcing every effect to reinit itself when used.
	// Architectural problem: how would Effect class figure out its assets no longer exist?
	// A good candidate for weak_ptr. If cache holds shared_ptr and Effect have a weak_ptr
	// it can figure out when cache is purged and reinit everything.
	//static void PurgeCache();

	// Parameters
	static float s_gameTime;

	// Standard parameters
	static std::map<std::string, unsigned> s_stdParams; // <"std parameter", std parameter id>

	// Caching collections
	static std::map<std::string, EffectFileCache*> s_fileCache;
	static std::map<unsigned int, EffectShader*> s_shaderCache;
	static std::map<unsigned int, EffectProgram*> s_programCache;
	static std::map<unsigned int, std::map<unsigned, unsigned>* > s_stdParamsCache;

	// Uniform blocks
	static std::vector<UniformBlock*> s_uniformBlocks;
	static std::map<std::string, int> s_uniformBlockNames; // <name, > Used to lookup a uniform block
};

// Check and warn about compile & link errors
static bool CheckErrors(const char *debug_desc, GLuint obj, const char* type = "shader")
{
	//check if shader or program
	bool isShader = (glIsShader(obj) == GL_TRUE);

	int infologLength = 0;
	char infoLog[1024];

	if (isShader)
		glGetShaderInfoLog(obj, 1024, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, 1024, &infologLength, infoLog);

	GLint status;
	if (isShader)
		glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
	else
		glGetProgramiv(obj, GL_LINK_STATUS, &status);

	if (status == GL_FALSE) {
		Error("Error compiling %s\n\n%s\n\n%sOpenGL vendor: %s\nOpenGL renderer string: %s",
			type, debug_desc, infoLog, glGetString(GL_VENDOR), glGetString(GL_RENDERER));
		return false;
	}

	// Log warnings even if successfully compiled
	// Sometimes the log is full of junk "success" messages so
	// this is not a good use for OS::Warning
#ifndef NDEBUG
	if (infologLength > 0) {
		if (pi_strcasestr("infoLog", "warning"))
			Output("%s: %s", debug_desc, infoLog);
	}
#endif

	return true;
}

}} // Namespace

#endif
