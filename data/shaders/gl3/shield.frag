// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

//scene uniform parameters
struct Scene {
	vec4 ambient;
};

// IN
in vec3 varyingEyepos;
in vec3 varyingNormal;
in vec3 varyingVertex;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;
uniform float shieldStrength;
uniform float shieldCooldown;
uniform Scene scene;

//#define MAX_SHIELD_HITS 5

// hitPos entries should be in object local space
uniform vec3 hitPos[MAX_SHIELD_HITS];
uniform float radii[MAX_SHIELD_HITS];
uniform int numHits;

//----------------------------------------------------- Fragment Shader
void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

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

	return clamp(1.0 / sqrDist * radius, 0.0, 0.9) * (1.0 - life);
}

void main(void)
{
	vec4 color = mix(red, blue, shieldStrength);
	vec4 fillColour = color * 0.15;
	
	vec3 eyenorm = normalize(-varyingEyepos);

	float fresnel = 1.0 - abs(dot(eyenorm, varyingNormal)); // Calculate fresnel.
	fresnel = pow(fresnel, 10.0);
	fresnel += 0.05 * (1.0 - fresnel);
	
	float sumIntensity = 0.0;
	for ( int hit = 0; hit < numHits; hit++)
	{
		sumIntensity += calcIntensity(hit);
	}
	float clampedInt = clamp(sumIntensity, 0.0, 1.0);
	
	// combine a base colour with the (clamped) fresnel value and fade it out according to the cooldown period.
	color.a = (fillColour.a + clamp(fresnel * 0.5, 0.0, 1.0)) * shieldCooldown;
	// add on our hit effect colour
	color = color + (hitColour * clampedInt);
	
	o_FragColor = color;

	SetFragDepth();
}
