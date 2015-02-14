// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: GEOSPHERE WITH TERRAIN + WATER/LAVA

//----------------------------------------------------- In/Out/Uniforms

// IN
in float varLogDepth;
in vec3 v_eyepos;
in vec3 v_normal;
in vec4 v_color;
in vec4 vertexColor;
#ifdef TERRAIN_WITH_LAVA
	in vec4 v_emission;
#endif // TERRAIN_WITH_LAVA

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;
uniform vec4 u_atmosColor;
// to keep distances sane we do a nearer, smaller scam. this is how many times
// smaller the geosphere has been made
uniform float u_geosphereScale;
uniform float u_geosphereScaledRadius;
uniform float u_geosphereAtmosTopRad;
uniform vec3 u_geosphereCenter;
uniform float u_geosphereAtmosFogDensity;
uniform float u_geosphereAtmosInvScaleHeight;
#ifdef ECLIPSE
	uniform int u_shadows;
	uniform ivec3 u_occultedLight;
	uniform vec3 u_shadowCentreX;
	uniform vec3 u_shadowCentreY;
	uniform vec3 u_shadowCentreZ;
	uniform vec3 u_lrad;
	uniform vec3 u_sdivlrad;
#endif // ECLIPSE

struct Material {
	vec4 diffuse;
	vec4 emission;
	vec4 specular;
	float shininess;
};
layout(std140) uniform UBMaterial {
	Material su_Material;
};

struct Scene {
	vec4 ambient;
};
uniform Scene scene;

#ifdef ECLIPSE
	#define PI 3.141592653589793

	float discCovered(const in float dist, const in float rad) {
		// proportion of unit disc covered by a second disc of radius rad placed
		// dist from centre of first disc.
		//
		// XXX: same function is in Camera.cpp
		//
		// WLOG, the second disc is displaced horizontally to the right.
		// xl = rightwards distance to intersection of the two circles.
		// xs = normalised leftwards distance from centre of second disc to intersection.
		// d = vertical distance to an intersection point
		//
		// The clamps on xl,xs handle the cases where one disc contains the other.
		float radsq = rad * rad;

		float xl = clamp((dist * dist + 1.0 - radsq) / (2.0 * max(0.001, dist)), -1.0, 1.0);
		float xs = clamp((dist - xl) / max(0.001, rad), -1.0, 1.0);
		float d = sqrt(max(0.0, 1.0 - xl * xl));

		float th = clamp(acos(xl), 0.0, PI);
		float th2 = clamp(acos(xs), 0.0, PI);

		// covered area can be calculated as the sum of segments from the two
		// discs plus/minus some triangles, and it works out as follows:
		return clamp((th + radsq*th2 - dist * d) / PI, 0.0, 1.0);
	}
#endif // ECLIPSE

// SHARED_UNIFORMS
#ifdef LIGHTING
	// light uniform parameters
	struct s_LightSourceParameters{
		vec4 position;
		vec4 diffuse;
		vec4 specular;
	};
	layout(std140) uniform UBLightSources {
		s_LightSourceParameters su_LightSource[4];
	};
	uniform float u_numLights;
#endif // LIGHTING

//----------------------------------------------------- Fragment Shader
void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

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

void main(void)
{
	vec3 eyepos = v_eyepos;
	vec3 eyenorm = normalize(eyepos);
	vec3 tnorm = normalize(v_normal);
	vec4 diff = vec4(0.0);
	float nDotVP = 0.0;
	float nnDotVP = 0.0;
	#ifdef ATMOSPHERE
		// when does the eye ray intersect atmosphere
		float atmosStart = findSphereEyeRayEntryDistance(u_geosphereCenter, eyepos, u_geosphereScaledRadius * u_geosphereAtmosTopRad);
	#endif // ATMOSPHERE
	float ldprod = 0.0;
	float fogFactor = 0.0;

	#ifdef TERRAIN_WITH_WATER
		float specularReflection=0.0;
	#endif // TERRAIN_WITH_WATER

	#ifdef LIGHTING
		float INV_NUM_LIGHTS = 1.0 / float(u_numLights);
		vec3 v = (eyepos - u_geosphereCenter) / u_geosphereScaledRadius;
		float lenInvSq = 1.0 / (dot(v, v));
		for (int i=0; i < u_numLights; ++i) {
			vec3 lightDir = normalize(vec3(su_LightSource[i].position));
			float unshadowed = 1.0;
			#ifdef ECLIPSE
				for (int j = 0; j < u_shadows; j++) {
					if (i != u_occultedLight[j]) {
						continue;
					}
					vec3 centre = vec3( u_shadowCentreX[j], u_shadowCentreY[j], u_shadowCentreZ[j]);
					// Apply eclipse:
					vec3 projectedPoint = v - dot(lightDir, v) * lightDir;
					// By our assumptions, the proportion of light blocked at this point by
					// this sphere is the proportion of the disc of radius u_lrad around
					// projectedPoint covered by the disc of radius srad around shadowCentre.
					float dist = length(projectedPoint - centre);
					unshadowed *= 1.0 - discCovered(dist / u_lrad[j], u_sdivlrad[j]);
				}
			#endif // ECLIPSE

			unshadowed = clamp(unshadowed, 0.0, 1.0);
			#ifdef ATMOSPHERE
				float fogNormalFactor = u_geosphereAtmosFogDensity * 80000.0; //fognormals should be 8 times atmosdenisty
				float atmosDist = u_geosphereScale * (length(eyepos) - atmosStart) * 0.5;

				{
					// a&b scaled so length of 1.0 means planet surface.
					vec3 a = (atmosStart * eyenorm - u_geosphereCenter) / u_geosphereScaledRadius;
					vec3 b = (eyepos - u_geosphereCenter) / u_geosphereScaledRadius;
					ldprod = AtmosLengthDensityProduct(a, b, u_atmosColor.w *u_geosphereAtmosFogDensity, atmosDist, u_geosphereAtmosInvScaleHeight);
					fogFactor = clamp( 1.25 / exp(ldprod), 0.0, 1.0);
				}

				vec3 surfaceNorm = mix(normalize(atmosStart * eyenorm - u_geosphereCenter), tnorm, fogFactor);

				vec3 n  = mix(tnorm, surfaceNorm, clamp(fogNormalFactor, 0.0, 1.0)); //mix eye normals in dense atmosphere.
				nDotVP  = max(0.0, dot(n, normalize(vec3(su_LightSource[i].position))));
				nnDotVP = max(0.0, dot(n, normalize(-vec3(su_LightSource[i].position)))); //need backlight to increase horizon
			#else
				nDotVP  = max(0.0, dot(tnorm, normalize(vec3(su_LightSource[i].position))));
				nnDotVP = max(0.0, dot(tnorm, normalize(-vec3(su_LightSource[i].position)))); //need backlight to increase horizon
			#endif // ATMOSPHERE
			diff += su_LightSource[i].diffuse * unshadowed * 0.5 * (nDotVP + 0.5 * clamp(1.0 - nnDotVP * 4.0, 0.0, 1.0) * INV_NUM_LIGHTS);

			#ifdef TERRAIN_WITH_WATER
					//Specular reflection
					vec3 L = normalize(su_LightSource[i].position.xyz - eyepos);
					vec3 E = normalize(-eyepos);
					vec3 R = normalize(-reflect(L,tnorm));
					//water only for specular
					if (vertexColor.b > 0.05 && vertexColor.r < 0.05) {
						specularReflection += pow(max(dot(R,E),0.0),16.0)*clamp(1.6-ldprod,0.6,1.6) * INV_NUM_LIGHTS;
					}
			#endif // TERRAIN_WITH_WATER
		}

	#ifdef ATMOSPHERE
		//calculate sunset tone red when passing through more atmosphere, clamp everything.
		float atmpower = (diff.r+diff.g+diff.b)/3.0;
		vec4 sunset = vec4(0.8,clamp(pow(atmpower,0.8),0.0,1.0),clamp(pow(atmpower,1.2),0.0,1.0),1.0);

		o_FragColor =
			su_Material.emission +
			#ifdef TERRAIN_WITH_LAVA
					v_emission +
			#endif
			fogFactor *
			((scene.ambient * vertexColor) +
			(diff * vertexColor)) +
			(1.0-fogFactor)*(diff*u_atmosColor) +
			#ifdef TERRAIN_WITH_WATER
				diff * specularReflection * sunset +
			#endif
			(0.02-clamp(fogFactor,0.0,0.01))*diff*ldprod*sunset +	      //increase fog scatter
			(pow((1.0-pow(fogFactor,0.75)),256.0)*0.4*diff*u_atmosColor)*sunset * //distant fog.
			clamp(1.0/sqrt(u_geosphereAtmosFogDensity*10000.0),0.4,1.0);  //darken atmosphere based on density
	#else // atmosphere-less planetoids and dim stars
		o_FragColor =
			su_Material.emission +
			#ifdef TERRAIN_WITH_LAVA
					v_emission +
			#endif // TERRAIN_WITH_LAVA
			(scene.ambient * vertexColor) +
			(diff * vertexColor * 2.0);
	#endif //ATMOSPHERE

#else // LIGHTING -- unlit rendering - stars
	//emission is used to boost colour of stars, which is a bit odd
	o_FragColor = su_Material.emission + vertexColor;
#endif // LIGHTING
	SetFragDepth();
}
