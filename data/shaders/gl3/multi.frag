// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: MULTI MATERIAL

//----------------------------------------------------- Predefs
#ifndef SHIELD_SHADER
	#define NORMAL_SHADER
#endif

//scene uniform parameters
struct Scene {
	vec4 ambient;
};

//----------------------------------------------------- In/Out/Uniforms
// IN
in float varLogDepth;

#ifdef TEXTURE0
	in vec2 texCoord0;
#endif // TEXTURE0

#ifdef VERTEXCOLOR
	in vec4 vertexColor;
#endif // VERTEXCOLOR

#ifdef LIGHTING
	in vec3 eyePos;
	in vec3 normal;
	#ifdef HEAT_COLOURING
		in vec3 heatingDir;
	#endif // HEAT_COLOURING
	#ifdef HEMISPHERE_LIGHT
		in vec3 viewDir;
		in vec3 worldNormal;
	#endif // HEMISPHERE_LIGHT
#endif // LIGHTING > 0

#ifdef SHIELD_SHADER
	in vec3 varyingEyepos;
	in vec3 varyingNormal;
	in vec3 varyingVertex;
#endif // SHIELD_SHADER

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;
#ifdef TEXTURE0
	uniform sampler2D texture0; //diffuse
	uniform sampler2D texture1; //specular
	uniform sampler2D texture2; //glow
	uniform sampler2D texture3; //pattern
	uniform sampler2D texture4; //color
#endif // TEXTURE0

#ifdef LIGHTING
	#ifdef HEAT_COLOURING
		uniform sampler2D heatGradient;
		uniform float heatingAmount; // 0.0 to 1.0 used for `u` component of heatGradient texture
	#endif // HEAT_COLOURING
#endif // LIGHTING

#ifdef COLOR_TINT
	uniform vec4 colorTint;
#endif // COLOR_TINT

#ifdef SHIELD_SHADER
	uniform float shieldStrength;
	uniform float shieldCooldown;

	#define MAX_SHIELD_HITS 5
	
	// hitPos entries should be in object local space
	uniform vec3 hitPos[MAX_SHIELD_HITS];
	uniform float radii[MAX_SHIELD_HITS];
	uniform int numHits;
#endif // SHIELD_SHADER

uniform Scene scene;

struct Material {
	vec4 diffuse;
	vec4 emission;
	vec4 specular;
	float shininess;
};
layout(std140) uniform UBMaterial {
	Material su_Material;
};

// SHARED_UNIFORMS
#ifdef LIGHTING
	// light uniform parameters
	struct s_LightSourceParameters {
		vec4 position;
		vec4 diffuse;
		vec4 specular;
	};	
	layout(std140) uniform UBLightSources {
		s_LightSourceParameters su_LightSource[MAX_NUM_LIGHTS];
	};
	uniform float u_numLights;
	#ifdef HEMISPHERE_LIGHT
		uniform samplerCube u_universeBox;
		uniform vec4 u_terrainColor;
		uniform vec4 u_atmosColor;
		uniform float u_atmosDensity;
	#endif
#endif // LIGHTING

//----------------------------------------------------- Fragment Shader
#ifdef SHIELD_SHADER
	const vec4 red = vec4(0.0, 0.0, 0.0, 0.0);
	const vec4 blue = vec4(0.0, 0.8, 1.0, 0.3);
	const vec4 hitColour = vec4(1.0, 0.5, 0.5, 1.0);

	float calcIntensity(int shieldIndex)
	{
		vec3 current_position = hitPos[shieldIndex];
		float life = radii[shieldIndex];
		float radius = 50.0 * life;
		vec3 dif = varyingVertex - current_position;
		
		float sqrDist = dot(dif,dif);

		return clamp(1.0/sqrDist*radius, 0.0, 0.9) * (1.0 - life);
	}
#endif // SHIELD_SHADER

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

#ifdef LIGHTING
	//ambient, diffuse, specular
	//would be a good idea to make specular optional
	void ads(in int lightNum, in vec3 pos, in vec3 n, inout vec4 light, inout vec4 specular,
		out float diffuse_coefficient, out float gloss_coefficient)
	{
		vec3 s = normalize(vec3(su_LightSource[lightNum].position)); //directional light
		vec3 v = normalize(vec3(-pos));
		vec3 h = normalize(v + s);
		diffuse_coefficient = max(dot(s, n), 0.0);
		gloss_coefficient = max(dot(h, n), 0.0);
		light += su_LightSource[lightNum].diffuse * su_Material.diffuse * diffuse_coefficient;
		float gloss_power = su_Material.shininess > 0.0? pow(gloss_coefficient, su_Material.shininess) : pow(gloss_coefficient, 80.0); // NVIDIA doesn't like when pow gets passed a zero
		#ifdef MAP_SPECULAR
			specular += texture(texture1, texCoord0) * su_Material.specular * su_LightSource[lightNum].diffuse * gloss_power;
		#else
			specular += su_Material.specular * su_LightSource[lightNum].diffuse * gloss_power;
		#endif // MAP_SPECULAR
		specular.a = 0.0;
		light.a = 1.0;
	}
#endif // LIGHTING

void main(void)
{
	#ifdef NORMAL_SHADER						// No Shield
		#ifdef VERTEXCOLOR
			vec4 color = vertexColor;
		#else
			vec4 color = su_Material.diffuse;
		#endif // VERTEXCOLOR
		#ifdef TEXTURE0
			color *= texture(texture0, texCoord0);
		#endif // TEXTURE0
		#ifdef MAP_COLOR
			//patterns - simple lookup
			vec4 pat = texture(texture3, texCoord0);
			vec4 mapColor = texture(texture4, vec2(pat.r, 0.0));
			vec4 tint = mix(vec4(1.0),mapColor,pat.a);
			color *= tint;
		#endif // MAP_COLOR

		#ifdef ALPHA_TEST
			if (color.a < 0.5)
				discard;
		#endif // ALPHA_TEST

		//directional lighting
		#ifdef LIGHTING
			vec4 light = scene.ambient +
			//ambient and emissive only make sense with lighting
			#ifdef MAP_EMISSIVE
					texture(texture2, texCoord0); //glow map
			#else
					su_Material.emission; //just emissive parameter
			#endif // MAP_EMISSIVE
			vec4 hemi_diff = vec4(0.0);
			vec4 hemi_gloss = vec4(0.0);
			#ifdef HEMISPHERE_LIGHT
				vec3 reflected_dir = reflect(viewDir, normal);
				float MIPlevel = log2(512.0 * sqrt(3.0)) - 0.5 * log2(4.0 + 1.0);
				float MaxMIPlevel = 5.0;
				hemi_diff = vec4(textureLod(u_universeBox, worldNormal, 
					MaxMIPlevel).rgb, 0.0);
				hemi_gloss = vec4(textureLod(u_universeBox, reflected_dir, 
					MIPlevel).rgb, 0.0);
			#endif
			vec4 specular = vec4(0.0);
			float diffuse_coefficient = 0.0;
			float gloss_coefficient = 0.0;
			for (int i=0; i<u_numLights; ++i) {
				float diff_coef;
				float gloss_coef;
				ads(i, eyePos, normal, light, specular, diff_coef, gloss_coef);
				diffuse_coefficient += diff_coef;
				gloss_coefficient += gloss_coef;
			}
			diffuse_coefficient = diffuse_coefficient / u_numLights;
			gloss_coefficient = gloss_coefficient / u_numLights;
			
			#ifdef HEMISPHERE_LIGHT
				// Indirect light should deminish when there is more light and
				// shine through when there is less.
				hemi_diff.rgb *= clamp((1.0 - diffuse_coefficient) * 0.35 * (1.0 - u_atmosDensity), 0.0, 1.0);
				hemi_gloss.rgb *= clamp((1.0 - gloss_coefficient) * 0.35 * (1.0 - u_atmosDensity), 0.0, 1.0);
			#endif // HEMISPHERE_LIGHT
			
			o_FragColor = color * (light + hemi_diff) + (specular + hemi_gloss);
			
			#ifdef HEAT_COLOURING
				if (heatingAmount > 0.0)
				{
					float dphNn = clamp(dot(heatingDir, normal), 0.0, 1.0);
					float heatDot = heatingAmount * (dphNn * dphNn * dphNn);
					vec4 heatColour = texture(heatGradient, vec2(heatDot, 0.5)); //heat gradient blend
					o_FragColor.rgb = o_FragColor.rgb + heatColour.rgb;
				}
			#endif // HEAT_COLOURING
		#else
			o_FragColor = color;
		#endif // LIGHTING

		#ifdef COLOR_TINT
			o_FragColor.rgb = o_FragColor.rgb * colorTint.rgb;
		#endif // COLOR_TINT
		
	#else 										// Shield shader
		vec4 color = mix(red, blue, shieldStrength);
		vec4 fillColour = color * 0.15;
		
		vec3 eyenorm = normalize(-varyingEyepos);

		float fresnel = 1.0 - abs(dot(eyenorm, varyingNormal)); // Calculate fresnel.
		fresnel = pow(fresnel, 10.0);
		fresnel += 0.05 * (1.0 - fresnel);
		
		float sumIntensity = 0.0;
		for ( int hit=0; hit<numHits; hit++ )
		{
			sumIntensity += calcIntensity(hit);
		}
		float clampedInt = clamp(sumIntensity, 0.0, 1.0);
		
		// combine a base colour with the (clamped) fresnel value and fade it out according to the cooldown period.
		color.a = (fillColour.a + clamp(fresnel * 0.5, 0.0, 1.0)) * shieldCooldown;
		// add on our hit effect colour
		color = color + (hitColour * clampedInt);
		
		o_FragColor = color;
	#endif // FRAGMENT_SHADER
	
	SetFragDepth();
}

