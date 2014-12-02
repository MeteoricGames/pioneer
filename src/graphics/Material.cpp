// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Material.h"

namespace Graphics {

Material::Material() :
	texture0(0),
	texture1(0),
	texture2(0),
	texture3(0),
	texture4(0),
	heatGradient(0),
	diffuse(Color::WHITE),
	specular(Color::BLACK),
	emissive(Color::BLACK),
	shininess(100), //somewhat sharp
	tint(Color::WHITE),
	specialParameter0(0),
	pointSize(1.0f),
	atmosphereColor(Color(0)),
	atmosphereDensity(0.0f)
{
}

MaterialDescriptor::MaterialDescriptor()
: effect(EFFECT_DEFAULT)
, alphaTest(false)
, glowMap(false)
, lighting(false)
, specularMap(false)
, usePatterns(false)
, vertexColors(false)
, pointsMode(false)
, colorTint(false)
, textures(0)
, dirLights(0)
, quality(0)
, testMode(false)
{
}

bool operator==(const MaterialDescriptor &a, const MaterialDescriptor &b)
{
	return (
		a.effect == b.effect &&
		a.alphaTest == b.alphaTest &&
		a.glowMap == b.glowMap &&
		a.lighting == b.lighting &&
		a.specularMap == b.specularMap &&
		a.usePatterns == b.usePatterns &&
		a.vertexColors == b.vertexColors &&
		a.pointsMode == b.pointsMode && 
		a.colorTint == b.colorTint && 
		a.textures == b.textures &&
		a.dirLights == b.dirLights &&
		a.quality == b.quality &&
		a.testMode == b.testMode
	);
}

}
