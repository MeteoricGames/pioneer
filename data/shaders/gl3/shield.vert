// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// IN
in vec4 a_Vertex;
in vec4 a_Normal;

// OUT
out vec3 varyingEyepos;
out vec3 varyingNormal;
out vec3 varyingVertex;

// UNIFORMS
uniform mat4 su_ModelViewProjectionMatrix;
uniform mat4 su_ModelViewMatrix;
uniform mat3 su_NormalMatrix;

//----------------------------------------------------- Vertex Shader
vec4 logarithmicTransform()
{
	vec4 vertexPosClip = su_ModelViewProjectionMatrix * a_Vertex;
	varLogDepth = vertexPosClip.z;
	return vertexPosClip;
}

void main(void)
{
	gl_Position = logarithmicTransform();

	varyingEyepos = vec3(u_ModelViewMatrix * a_Vertex);
	varyingNormal = normalize(u_NormalMatrix * a_Normal);
	varyingVertex = a_Vertex.xyz;
}
