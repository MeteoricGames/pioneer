// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: MULTI MATERIAL

//----------------------------------------------------- Predefs
#ifndef SHIELD_SHADER
	#define NORMAL_SHADER
#endif


//----------------------------------------------------- In/Out/Uniforms

// IN
in vec4 a_Vertex;
in vec4 a_Normal;
#ifdef NORMAL_SHADER							// Not Shield
	in vec4 a_Color;
	in vec2 a_MultiTexCoord0;
#endif // NORMAL_SHADER

// OUT
out float varLogDepth;

#ifdef TEXTURE0
	out vec2 texCoord0;
#endif // TEXTURE0

#ifdef VERTEXCOLOR
	out vec4 vertexColor;
#endif // VERTEX_COLOR

#ifdef LIGHTING
	out vec3 eyePos;
	out vec3 normal;
	#ifdef HEAT_COLOURING
		out vec3 heatingDir;
	#endif // HEAT_COLOURING
	#ifdef HEMISPHERE_LIGHT
		out vec3 viewDir;
		out vec3 worldNormal;
	#endif // HEMISPHERE_LIGHT
#endif // LIGHTING

#ifdef SHIELD_SHADER
	out vec3 varyingEyepos;
	out vec3 varyingNormal;
	out vec3 varyingVertex;
#endif // SHIELD_SHADER

// UNIFORMS
uniform mat4 su_ModelViewProjectionMatrix;
uniform mat4 su_ModelViewMatrix;
uniform mat3 su_NormalMatrix;

#ifdef LIGHTING
	#ifdef HEAT_COLOURING
		uniform mat3 heatingMatrix;
		uniform vec3 heatingNormal; // normalised
	#endif // HEAT_COLOURING
	#ifdef HEMISPHERE_LIGHT
		uniform mat4 su_ViewMatrixInverse;
	#endif // HEMISPHERE_LIGHT
#endif // LIGHTING

#ifdef POINTS
	uniform float pointSize;
#endif // POINTS

// SHARED_UNIFORMS
#ifdef LIGHTING
	// light uniform parameters
	struct s_LightSourceParameters {
		vec4 position;
		vec4 diffuse;
		vec4 specular;
	};
	layout(std140) uniform UBLightSources {
		s_LightSourceParameters su_LightSource [MAX_NUM_LIGHTS];
	};
	uniform float u_numLights;
#endif // LIGHTING

//----------------------------------------------------- Vertex Shader
vec4 logarithmicTransform()
{
	vec4 vertexPosClip = su_ModelViewProjectionMatrix * a_Vertex;
	varLogDepth = vertexPosClip.z;
	return vertexPosClip;
}

void main(void)
{
	// Prevent GL from optimizing out the normal attribute
	a_Normal; // Should use glBindAttribLocation instead for this situation
	gl_Position = logarithmicTransform();
#ifdef NORMAL_SHADER
	#ifdef VERTEXCOLOR
		vertexColor = a_Color;
	#endif
	#ifdef TEXTURE0
		texCoord0 = a_MultiTexCoord0.xy;
	#endif
	#ifdef LIGHTING
		eyePos = vec3(su_ModelViewMatrix * a_Vertex);
		//normal = normalize(su_NormalMatrix * a_Normal.xyz);
		normal = normalize(su_ModelViewMatrix * a_Normal).xyz;
		#ifdef HEAT_COLOURING
			heatingDir = normalize(heatingMatrix * heatingNormal);
		#endif
		#ifdef HEMISPHERE_LIGHT
			viewDir = (su_ViewMatrixInverse * vec4(normalize(-eyePos), 1.0)).xyz;
			worldNormal = a_Normal.xyz;
		#endif
	#endif
	#ifdef POINTS
		gl_PointSize = pointSize;
	#endif
#else
	varyingEyepos = vec3(su_ModelViewMatrix * a_Vertex);
	varyingNormal = normalize(su_NormalMatrix * a_Normal.xyz);
	varyingVertex = a_Vertex.xyz;
#endif
}
