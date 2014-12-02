// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// Material uniform parameters
struct Material {
	vec4 diffuse;
	vec4 emission;
	vec4 specular;
	float shininess;
};

struct s_MaterialParameters {
	vec4 emission; // Ecm
	vec4 ambient; // Acm
	vec4 diffuse; // Dcm
	vec4 specular; // Scm
	float shininess; // Srm
};

// light uniform parameters
struct s_LightSourceParameters{
	vec4 ambient; // Acli
	vec4 diffuse; // Dcli
	vec4 specular; // Scli
	vec4 position; // Ppli
	vec4  halfVector; // Derived: Hi
	vec3  spotDirection; // Sdli
	float spotExponent; // Srli
	float spotCutoff; // Crli
	// (range: [0.0,90.0], 180.0)
	float spotCosCutoff; // Derived: cos(Crli)
	// (range: [1.0,0.0],-1.0)
	float constantAttenuation; // K0
	float linearAttenuation; // K1
	float quadraticAttenuation;// K2
};
uniform s_LightSourceParameters u_LightSource[NUM_LIGHTS];

#ifdef FRAGMENT_SHADER
//scene uniform parameters
struct Scene {
	vec4 ambient;
};

//Currently used by: planet ring shader, geosphere shaders
float findSphereEyeRayEntryDistance(in vec3 sphereCenter, in vec3 eyeTo, in float radius)
{
	vec3 v = -sphereCenter;
	vec3 dir = normalize(eyeTo);
	float b = -dot(v, dir);
	float det = (b * b) - dot(v, v) + (radius * radius);
	float entryDist = 0.0;
	if (det > 0.0) {
		det = sqrt(det);
		float i1 = b - det;
		float i2 = b + det;
		if (i2 > 0.0) {
			entryDist = max(i1, 0.0);
		}
	}
	return entryDist;
}

// Used by: geosphere shaders
// Calculate length*density product of a line through the atmosphere
// a - start coord (normalized relative to atmosphere radius)
// b - end coord " "
// centerDensity - atmospheric density at centre of sphere
// length - real length of line in meters
float AtmosLengthDensityProduct(vec3 a, vec3 b, float surfaceDensity, float len, float invScaleHeight)
{
	/* 4 samples */
	float ldprod = 0.0;
	vec3 dir = b-a;
	ldprod = surfaceDensity * (
			exp(-invScaleHeight*(length(a)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.2*dir)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.4*dir)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.6*dir)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.8*dir)-1.0)) +
			exp(-invScaleHeight*max(length(b)-1.0, 0.0)));
	ldprod *= len;
	return ldprod;
}
#endif
