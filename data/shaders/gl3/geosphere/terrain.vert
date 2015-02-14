// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: TERRAIN

//----------------------------------------------------- In/Out/Uniforms

// IN
in vec4 a_Vertex;
in vec4 a_Normal;
in vec4 a_Color;

// OUT
out float varLogDepth;
out vec3 v_eyepos;
out vec3 v_normal;
out vec4 v_color;
#ifdef TERRAIN_WITH_LAVA
	out vec4 v_emission;
#endif
out vec4 vertexColor;

// UNIFORMS
uniform mat4 su_ModelViewMatrix;
uniform mat4 su_ModelViewProjectionMatrix;
uniform mat3 su_NormalMatrix;
#ifdef TERRAIN_WITH_LAVA
struct Material {
	vec4 diffuse;
	vec4 emission;
	vec4 specular;
	float shininess;
};
layout(std140) uniform UBMaterial {
	Material su_Material;
};
#endif

//----------------------------------------------------- Vertex Shader
vec4 logarithmicTransform()
{
	vec4 vertexPosClip = su_ModelViewProjectionMatrix * a_Vertex;
	varLogDepth = vertexPosClip.z;
	return vertexPosClip;
}

void main(void)
{
	a_Normal;
	gl_Position = logarithmicTransform();
	vertexColor = a_Color;
	v_eyepos = vec3(su_ModelViewMatrix * a_Vertex);
	v_normal = su_NormalMatrix * a_Normal.xyz;

#ifdef TERRAIN_WITH_LAVA
	v_emission = su_Material.emission;
	// Glow lava terrains
	if (vertexColor.r > 0.4 && vertexColor.g < 0.2 && vertexColor.b < 0.4) {
		v_emission = 3.0 * vertexColor;
		v_emission *= (vertexColor.r + vertexColor.g + vertexColor.b);

	}
#endif
}
