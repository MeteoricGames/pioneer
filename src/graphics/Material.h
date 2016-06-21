// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MATERIAL_H
#define _MATERIAL_H
/*
 * Materials are used to apply an appropriate shader and other rendering parameters.
 * Users request materials from the renderer by filling a MaterialDescriptor structure,
 * and calling Renderer::CreateMaterial.
 * Users are responsible for deleting a material they have requested. This is because materials
 * are rarely shareable.
 * Material::Apply is called by renderer before drawing, and Unapply after drawing (to restore state).
 * For the GL2 renderer, a Material is always accompanied by a Program.
 */
#include "libs.h"
#include <RefCounted.h>

namespace Graphics {

class Texture;
class RendererGL2;

namespace GL3 { class Effect; } 

// Shorthand for unique effects
// The other descriptor parameters may or may not have effect,
// depends on the effect
enum EffectType {
	EFFECT_DEFAULT,
	EFFECT_STARFIELD,
	EFFECT_PLANETRING,
	EFFECT_GEOSPHERE_TERRAIN,
	EFFECT_GEOSPHERE_TERRAIN_WITH_LAVA,
	EFFECT_GEOSPHERE_TERRAIN_WITH_WATER,
	EFFECT_GEOSPHERE_SKY,
	EFFECT_FRESNEL_SPHERE,
	EFFECT_SKYBOX,
	EFFECT_TEXTURED_FULLSCREEN_QUAD,
	EFFECT_HORIZONTAL_BLUR,
	EFFECT_SHIELD,
	EFFECT_SPHEREIMPOSTOR,
	EFFECT_VERTICAL_BLUR,
	EFFECT_RADIAL_BLUR,
	EFFECT_BLOOM_COMPOSITOR,
	EFFECT_THRUSTERTRAILS_DEPTH,
	EFFECT_THRUSTERTRAILS,
	EFFECT_SECTORVIEW_ICON,
	EFFECT_TRANSIT_TUNNEL,
	EFFECT_TRANSIT_COMPOSITION,
};


// XXX : there must be a better place to put this
enum MaterialQuality {
	HAS_ATMOSPHERE		= 1 << 0,
	HAS_ECLIPSES		= 1 << 1,
	HAS_HEAT_GRADIENT   = 1 << 2
};

// Renderer creates a material that best matches these requirements.
// EffectType may override some of the other flags.
class MaterialDescriptor {
public:
	MaterialDescriptor();
	EffectType effect;
	bool alphaTest;
	bool glowMap;
	bool lighting;
	bool specularMap;
	bool usePatterns; //pattern/color system
	bool vertexColors;
	bool pointsMode;
	bool colorTint;
	Sint32 textures; //texture count
	Uint32 dirLights; //set by rendererGL2 if lighting == true
	Uint32 quality; // see: Graphics::MaterialQuality
	bool testMode;
	bool irradiance;

	friend bool operator==(const MaterialDescriptor &a, const MaterialDescriptor &b);
};

struct MaterialBlock
{
	MaterialBlock(const vector4f& _diffuse, const vector4f& _emission, const vector3f& _specular, float _shininess)
		: diffuse(_diffuse), emission(_emission), specular(_specular), shininess(_shininess) { }
	MaterialBlock(const Color& _diffuse, const Color& _emission, const Color& _specular, float _shininess)
	{
		diffuse = _diffuse.ToVector4f();
		emission = _emission.ToVector4f();
		specular = _specular.ToVector3f();
		shininess = _shininess;
	}
	vector4f diffuse;
	vector4f emission;
	vector3f specular;
	float shininess;
};

/*
 * A generic material with some generic parameters.
 */
class Material : public RefCounted {
public:
	Material();
	virtual ~Material() { }

	Texture *texture0;
	Texture *texture1;
	Texture *texture2;
	Texture *texture3;
	Texture *texture4;
	Texture *heatGradient;

	Color diffuse;
	Color specular;
	Color emissive;
	Color tint;
	Color atmosphereColor;

	int shininess; //specular power 0-128
	float pointSize;
	float atmosphereDensity;

	virtual void Apply() { }
	virtual void Unapply() { }
	virtual GL3::Effect* GetEffect() const { assert(false); return nullptr; }

	void *specialParameter0; //this can be whatever. Bit of a hack.

	//XXX may not be necessary. Used by newmodel to check if a material uses patterns
	const MaterialDescriptor &GetDescriptor() const { return m_descriptor; }

protected:
	MaterialDescriptor m_descriptor;

private:
	friend class RendererGL2;
};

}

#endif
