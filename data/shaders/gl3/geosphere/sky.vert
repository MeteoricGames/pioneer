// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: GEOSPHERE SKY

//----------------------------------------------------- In/Out/Uniforms

// IN
in vec4 a_Vertex;

// OUT
out float varLogDepth;
out vec4 v_eyepos;

// UNIFORMS
uniform mat4 su_ModelViewMatrix;
uniform mat4 su_ModelViewProjectionMatrix;

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
	v_eyepos = su_ModelViewMatrix * a_Vertex;
}
