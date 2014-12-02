// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Effect.h"
#include "FileSystem.h"
#include "graphics/Renderer.h"
#include "utils.h"
#include "EffectShader.h"
#include "EffectProgram.h"
#include "StringF.h"
#include "graphics/Material.h"

const std::string ShadersPath("shaders/");
const std::string VertexShaderDef("#define VERTEX_SHADER 1\n");
const std::string FragmentShaderDef("#define FRAGMENT_SHADER 1\n");
const std::string GeometryShaderDef("#define GEOMETRY_SHADER 1\n");
const std::string DebugPragmas("#pragma optimize(off)\n#pragma debug(on)\n");
const std::string DebugDef("#define DEBUG\n");
const std::string MaxLightsDef(stringf("#define MAX_NUM_LIGHTS %0{u}\n", static_cast<unsigned>(MATERIAL_MAX_LIGHTS)));

using namespace Graphics::GL3;

namespace Graphics { namespace GL3 {

bool Effect::s_isInit = false;
std::map<std::string, EffectFileCache*> Effect::s_fileCache;
std::map<unsigned int, EffectShader*> Effect::s_shaderCache;
std::map<unsigned int, EffectProgram*> Effect::s_programCache;
std::map<unsigned int, std::map<unsigned, unsigned>* > Effect::s_stdParamsCache;
std::map<std::string, unsigned> Effect::s_stdParams;
std::vector<Effect::UniformBlock*> Effect::s_uniformBlocks;
std::map<std::string, int> Effect::s_uniformBlockNames;

Effect::Effect(Renderer* renderer, const EffectDescriptor& effect_desc) :
	m_renderer(renderer), m_isDirect(false), m_effectDesc(effect_desc), 
	m_vertexShader(nullptr), m_fragmentShader(nullptr), m_program(nullptr),
	m_curSettingsHash(0), m_curVSHash(0), m_curFSHash(0), m_attribsStride(0),
	m_isApplied(false), m_isProgramUsed(false)
{
	assert(m_renderer);
	assert(!effect_desc.vertex_shader.empty() && !effect_desc.fragment_shader.empty());

	if(!Effect::s_isInit) {
		Effect::StaticInit();
	}

	for(unsigned i = 0; i < m_effectDesc.settings.size(); ++i) {
		if(m_effectDesc.settings[i][0] != '#') {
			m_effectDesc.settings[i] = "#define " + m_effectDesc.settings[i];
		}
		m_effectDesc.settings[i] += "\n";
	}
	m_curVSHash = CalculateVSHash();
	m_curFSHash = CalculateFSHash();
	m_curSettingsHash = CalculateSettingsHash(m_effectDesc.settings);

	// Create program and initialize it
	m_vertexDebugDesc ="\nVertex shader: " + m_effectDesc.vertex_shader;
	m_fragmentDebugDesc = "\nFragment shader: " + m_effectDesc.fragment_shader;
	m_programDebugDesc = "\nVertex shader: " + m_effectDesc.vertex_shader +
		"\nFragment shader: " + m_effectDesc.fragment_shader;

	Init();
}

Effect::Effect(Renderer* renderer, const EffectDescriptorDirect& effect_desc) :
	m_renderer(renderer), m_isDirect(true), m_effectDescDirect(effect_desc),
	m_vertexShader(nullptr), m_fragmentShader(nullptr), m_program(nullptr),
	m_curSettingsHash(0), m_curVSHash(0), m_curFSHash(0), m_attribsStride(0),
	m_isApplied(false), m_isProgramUsed(false)
{
	assert(m_renderer);
	assert(!effect_desc.vertex_shader.empty() && !effect_desc.fragment_shader.empty());

	if (!Effect::s_isInit) {
		Effect::StaticInit();
	}

	m_effectDesc.version = effect_desc.version;
	m_effectDesc.settings = effect_desc.settings;
	m_effectDesc.vertex_shader = effect_desc.vertex_shader_debug_name;
	m_effectDesc.fragment_shader = effect_desc.fragment_shader_debug_name;
	m_effectDesc.uniforms = effect_desc.uniforms;

	for (unsigned i = 0; i < m_effectDesc.settings.size(); ++i) {
		if (m_effectDesc.settings[i][0] != '#') {
			m_effectDesc.settings[i] = "#define " + m_effectDesc.settings[i];
		}
		m_effectDesc.settings[i] += "\n";
	}
	m_curVSHash = CalculateVSHash();
	m_curFSHash = CalculateFSHash();
	m_curSettingsHash = CalculateSettingsHash(m_effectDesc.settings);

	// Create program and initialize it
	m_vertexDebugDesc = "\nVertex shader: " + m_effectDesc.vertex_shader;
	m_fragmentDebugDesc = "\nFragment shader: " + m_effectDesc.fragment_shader;
	m_programDebugDesc = "\nVertex shader: " + m_effectDesc.vertex_shader +
		"\nFragment shader: " + m_effectDesc.fragment_shader;

	Init();
}

Effect::~Effect()
{

}

void Effect::StaticInit()
{
	if(s_isInit) { 
		return;
	}
	s_isInit = true;
	for(unsigned i = 0; i < StandardParameters.size(); ++i) {
		s_stdParams.insert(std::make_pair(StandardParameters[i], i));
	}
}

/*void Effect::PurgeCache()
{
	for(auto const & p : s_programCache) {
		delete p.second;
	}
	s_programCache.clear();
	for(auto const & s : s_shaderCache) {
		delete s.second;
	}
	s_shaderCache.clear();
	for(auto const & f : s_fileCache) {
		delete f.second;
	}
	s_fileCache.clear();
}*/

int Effect::DefineSharedUniformBlock(const char* name, size_t data_size)
{
	GLuint ubuffer;
	int binding_point = s_uniformBlocks.size() + 1;
	glGenBuffers(1, &ubuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, ubuffer);
	glBufferData(GL_UNIFORM_BUFFER, data_size, 0, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, ubuffer);

	Effect::UniformBlock* ub = new Effect::UniformBlock();
	ub->binding_point = binding_point;
	ub->data_size = data_size;
	ub->name = name;
	ub->ubuffer = ubuffer;
	s_uniformBlocks.push_back(ub);
	s_uniformBlockNames.insert(std::make_pair(name, binding_point));

	assert(binding_point == s_uniformBlocks.size());

	return binding_point;
}

int Effect::GetSharedUniformBlock(const char* name)
{
	assert(s_uniformBlockNames.count(name) > 0);
	return s_uniformBlockNames.at(name);
}

int Effect::InitUniformBlock(int uniform_block)
{
	if(uniform_block > static_cast<int>(s_uniformBlocks.size())) {
		Error("Effect::InitUniformBlock -> Shared UniformBlock %d has not been defined. Define block using Effect::DefineSharedUniformBlock()", uniform_block);
		assert(false);
	}
	GLuint pid = m_program->GetProgramID();
	int u = uniform_block - 1;
	int block_index = glGetUniformBlockIndex(pid, s_uniformBlocks[u]->name.c_str());
	if(block_index > -1) {
		glUniformBlockBinding(pid, block_index, s_uniformBlocks[u]->binding_point);
		return uniform_block;
	} else {
		return -1;
	}
	// If block index is not found it'll fail silently since there is no explicit 
	// requirement for existence of shared uniform blocks.
	
}

void Effect::WriteUniformBlock(int uniform_block, size_t data_size, void* data)
{
	if(uniform_block < 0) {
		return;
	}
	assert(data_size > 0 && data);
	if(uniform_block > static_cast<int>(s_uniformBlocks.size())) {
		Error("Effect::WriteUniformBlock -> Shared UniformBlock %d has not been defined.", uniform_block);
		assert(false);
	}
	int u = uniform_block - 1;
	if(data_size > s_uniformBlocks[u]->data_size) {
		Error("Uniform block data size too large while attempting to write to uniform block: \"%s\"", 
			s_uniformBlocks[u]->name.c_str());
	}
	size_t write_size = std::min(data_size, s_uniformBlocks[u]->data_size);
	assert(write_size > 0);
	glBindBuffer(GL_UNIFORM_BUFFER, s_uniformBlocks[u]->ubuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, write_size, data);
}

void Effect::ChangeSettings(const std::vector<std::string>& new_settings)
{
	unsigned int sh = CalculateSettingsHash(new_settings);
	if(sh != m_curSettingsHash) {
		// Settings changes detected
		m_curSettingsHash = sh;
		Init();
	}
}

Attribute& Effect::GetAttribute(EEffectAttributes attrib) const
{
	return (*m_attribs[static_cast<unsigned int>(attrib)]);
}

unsigned int Effect::GetProgramID() const
{
	return m_program->GetProgramID();
}

unsigned int Effect::GetUniformID(const char* name) const
{
	auto u_iter = m_uniformIndices.find(name);
	if(u_iter != m_uniformIndices.end()) {
		return u_iter->second;
	} else {
		if(m_effectDesc.strict) {
			Error("GetUniformID() could not find uniform \"%s\". Effect Info: %s", name, GetDebugString());
		}
		return -1;
	}
}

Graphics::GL3::Uniform& Effect::GetUniform(unsigned int id) const
{
	assert(id != -1 && id < m_uniforms.size());
	assert(IsProgramUsed()); // This is a guard, if you try to set uniforms before effect is applied rendering will glitch
	return *m_uniforms[id];
}

const char* Effect::GetDebugString() const
{
	return m_vertexDebugDesc.c_str();
}

void Effect::Init()
{
	m_vertexShader = InitShader(EffectShaderType::EST_VERTEX, m_curVSHash + m_curSettingsHash);
	m_fragmentShader = InitShader(EffectShaderType::EST_FRAGMENT, m_curFSHash + m_curSettingsHash);
	m_program = InitProgram(m_curVSHash + m_curFSHash + m_curSettingsHash);
	SetProgram();
	InitAttribs();
	InitUniforms(m_curVSHash + m_curFSHash + m_curSettingsHash);
	UnsetProgram();
}

EffectProgram* Effect::InitProgram(unsigned int hash)
{
	auto p_iter = s_programCache.find(hash);
	if(p_iter == s_programCache.end()) {
		EffectProgram* ep = new EffectProgram(m_vertexShader, m_fragmentShader, m_programDebugDesc);
		s_programCache.insert(std::pair<unsigned int, EffectProgram*>(hash, ep));
		return ep;
	} else {
		return p_iter->second;
	}
}

void Effect::InitAttribs()
{
	m_attribs.clear();
	m_attribsStride = 0;
	for(unsigned i = 0; i < EffectAttributes.size(); ++i) {
		Attribute* a = new Attribute();
		a->Init(EffectAttributes[i].c_str(), m_program->GetProgramID());
		m_attribs.push_back(a);
		if(a->GetLocation() != -1) {
			m_attribsStride += EffectAttributesSizes[i];
		}
	}
}

void Effect::InitUniforms(unsigned int hash)
{
	m_uniforms.clear();
	m_uniformIndices.clear();
	m_stdParams = nullptr;

	// Check standard parameters cache
	auto sp_iter = s_stdParamsCache.find(hash);
	bool sp_cached = sp_iter != s_stdParamsCache.end();
	if(sp_cached) {
		m_stdParams = sp_iter->second;
	} else {
		m_stdParams = new std::map<unsigned, unsigned>();
	}

	for (unsigned i = 0; i < m_effectDesc.uniforms.size(); ++i) {
		// Uniforms and indices
		Graphics::GL3::Uniform* u = new Graphics::GL3::Uniform();
		u->Init(m_effectDesc.uniforms[i].c_str(), m_program->GetProgramID());
		if(m_effectDesc.strict && u->m_location == -1) {
			Error("Failed to initialize uniform \"%s\". Uniform could not be found: %s",
				m_effectDesc.uniforms[i].c_str(), GetDebugString());
		}
		m_uniforms.push_back(u);
		m_uniformIndices.insert(std::pair<const char*, unsigned int>(
			m_effectDesc.uniforms[i].c_str(), i));

		if(!sp_cached) {
			// Standard parameter
			auto spi = s_stdParams.find(m_effectDesc.uniforms[i]);
			if(spi != s_stdParams.end()) { 
				// Found one.
				m_stdParams->insert(std::make_pair(spi->second, i));
			}
		}
	}

	if(!sp_cached) {
		s_stdParamsCache.insert(std::make_pair(hash, m_stdParams));
	}
}

EffectShader* Effect::InitShader(EffectShaderType type, unsigned int hash)
{
	auto pc_iter = s_shaderCache.find(hash);
	if (pc_iter == s_shaderCache.end()) {
		std::vector<const char*> blocks;
		std::vector<int> lengths;

		// GLSL Version
		std::string glsl_version(GLSLVersions[static_cast<unsigned int>(m_effectDesc.version)]);
		blocks.push_back(glsl_version.c_str());
		lengths.push_back(glsl_version.length());
		// GLSL debug and optimization
#ifndef NDEBUG
		blocks.push_back(DebugPragmas.c_str());
		lengths.push_back(DebugPragmas.length());
		blocks.push_back(DebugDef.c_str());
		lengths.push_back(DebugDef.length());
#endif
		// Vertex/Fragment definition
		if (type == EffectShaderType::EST_VERTEX) {
			blocks.push_back(VertexShaderDef.c_str());
			lengths.push_back(VertexShaderDef.length());
		} else if (type == EffectShaderType::EST_FRAGMENT) {
			blocks.push_back(FragmentShaderDef.c_str());
			lengths.push_back(FragmentShaderDef.length());
		}
		// Game settings
		blocks.push_back(MaxLightsDef.c_str());
		lengths.push_back(MaxLightsDef.length());
		// Effect settings
		for (unsigned i = 0; i < m_effectDesc.settings.size(); ++i) {
			blocks.push_back(m_effectDesc.settings[i].c_str());
			lengths.push_back(m_effectDesc.settings[i].length());
		}
		// Libraries (Removed for redesigning)
		/*for (unsigned i = 0; i < m_effectDesc.header_shaders.size(); ++i) {
			efc = LoadShader(m_effectDesc.header_shaders[i]);
			blocks.push_back(efc->data);
			lengths.push_back(efc->length);
		}*/

		// Vertex/Fragment shaders
		if (!m_isDirect) {
			EffectFileCache* efc = nullptr;
			if (type == EffectShaderType::EST_VERTEX) {
				efc = LoadShader(m_effectDesc.vertex_shader);
			} else {
				efc = LoadShader(m_effectDesc.fragment_shader);
			}
			blocks.push_back(efc->data);
			lengths.push_back(efc->length);
		} else {
			if(type == EffectShaderType::EST_VERTEX) {
				blocks.push_back(m_effectDescDirect.vertex_shader.c_str());
				lengths.push_back(m_effectDescDirect.vertex_shader.length());
			} else {
				blocks.push_back(m_effectDescDirect.fragment_shader.c_str());
				lengths.push_back(m_effectDescDirect.fragment_shader.length());
			}
		}

		// Create effect shader
		EffectShader* shader = new EffectShader(blocks, lengths, type,
			type == EffectShaderType::EST_VERTEX? m_vertexDebugDesc : m_fragmentDebugDesc);

		// Cache shader
		s_shaderCache.insert(std::pair<unsigned int, EffectShader*>(hash, shader));

		return shader;

	} else {
		// existing shader
		return pc_iter->second;
	}
}

unsigned int Effect::CalculateSettingsHash(const std::vector<std::string>& settings)
{
	unsigned int ho = 0;
	for (unsigned i = 0; i < settings.size(); ++i) {
		ho += CalculateHash(settings[i].c_str());
	}
	return ho;
}

unsigned int Effect::CalculateVSHash()
{
	return CalculateHash(m_effectDesc.vertex_shader.c_str());
}

unsigned int Effect::CalculateFSHash()
{
	return CalculateHash(m_effectDesc.fragment_shader.c_str()) + 11;
}

unsigned int Effect::CalculateHash(const char* s, unsigned int seed)
{
	unsigned hash = seed;
	while (*s) {
		hash = hash * 101 + *s++;
	}
	return hash;
}

EffectFileCache* Effect::LoadShader(const std::string& shader_file)
{
	auto sf_iter = s_fileCache.find(shader_file);
	if(sf_iter != s_fileCache.end()) {
		// File in cache!
		return sf_iter->second;
	} else {
		// Load file
		RefCountedPtr<FileSystem::FileData> file = 
			FileSystem::gameDataFiles.ReadFile(ShadersPath + shader_file);
		assert(file);
		StringRange str = file->AsStringRange().StripUTF8BOM();
		// Cache file
		auto efc = new EffectFileCache();
		efc->data = new char [str.Size() + 1];
		std::strncpy(efc->data, str.begin, sizeof(char) * str.Size());
		efc->data[str.Size()] = '\0';
		efc->length = str.Size();
		s_fileCache.insert(std::pair<std::string, EffectFileCache*>(shader_file, efc));
		return efc;
	}
}

void Effect::SetProgram()
{
	m_program->Use();
	m_isProgramUsed = true;
}

void Effect::UnsetProgram()
{
	m_program->Unuse();
	m_isProgramUsed = false;
}

void Effect::Apply()
{
	if(!m_isApplied) {
		m_isApplied = true;
		SetProgram();
	}

	// Standard parameters
	if (m_stdParams->size() > 0) {
		UpdateStandardParameters();
	}
}

void Effect::Unapply()
{
	assert(m_isApplied == true);
	UnsetProgram();
	m_isApplied = false;
}

void Effect::UpdateStandardParameters()
{
	for(auto& kv : (*m_stdParams)) {
		switch(kv.first) {
			case 0: // ModelViewMatrix
				m_uniforms[kv.second]->Set(m_renderer->GetCurrentModelView());
				break;

			case 1: // ProjectionMatrix
				m_uniforms[kv.second]->Set(m_renderer->GetCurrentProjection());
				break;

			case 2: // ModelViewProjectionMatrix
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewProjectionMatrix());
				break;

			case 3: // NormalMatrix
				m_uniforms[kv.second]->Set(m_renderer->CalcNormalMatrix());
				break;

			case 4: // ModelViewMatrixInverse
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewMatrixInverse());
				break;

			case 5: // ProjectionMatrixInverse
				m_uniforms[kv.second]->Set(m_renderer->CalcProjectionMatrixInverse());
				break;

			case 6: // ModelViewProjectionMatrixInverse
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewProjectionMatrixInverse());
				break;

			case 7: // ModelViewMatrixTranspose
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewMatrixTranspose());
				break;

			case 8: // ProjectionMatrixTranspose
				m_uniforms[kv.second]->Set(m_renderer->CalcProjectionMatrixTranspose());
				break;

			case 9: // ModelViewProjectionTranspose
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewProjectionMatrixTranspose());
				break;

			case 10: // ModelViewMatrixInverseTranspose
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewMatrixInverseTranspose());
				break;

			case 11: // ProjectionMatrixInverseTranspose
				m_uniforms[kv.second]->Set(m_renderer->CalcProjectionMatrixInverseTranspose());
				break;

			case 12: // ModelViewProjectionMatrixInverseTranspose
				m_uniforms[kv.second]->Set(m_renderer->CalcModelViewProjectionMatrixInverseTranspose());
				break;

			case 13: // ViewMatrix
				m_uniforms[kv.second]->Set(m_renderer->CalcViewMatrix());
				break;

			case 14: // ViewMatrixInverse
				m_uniforms[kv.second]->Set(m_renderer->CalcViewMatrixInverse());
				break;
		}
	}
}


}} // Namespace
