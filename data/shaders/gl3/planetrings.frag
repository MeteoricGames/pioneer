// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// IN
in float varLogDepth;
in vec4 v_TexCoord0;
in vec4 v_TexCoord1;
in vec4 v_FrontColor;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;
uniform sampler2D texture0;
uniform mat4 u_ModelViewMatrixInverse;

// SHARED_UNIFORMS
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

//----------------------------------------------------- Fragment Shader
void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

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

void main(void)
{
	// Bits of ring in shadow!
	vec4 col = vec4(0.0);
	vec4 texCol = texture(texture0, v_TexCoord0.st);

	for (int i = 0; i < u_numLights; ++i) {
		float l = findSphereEyeRayEntryDistance(-vec3(v_TexCoord1), vec3(u_ModelViewMatrixInverse * su_LightSource[i].position), 1.0);
		if (l <= 0.0) {
			col = col + texCol * su_LightSource[i].diffuse;
		}
	}
	
	col.a = texCol.a;
	o_FragColor = col;

	SetFragDepth();
}
