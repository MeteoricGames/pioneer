// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: TEST

//----------------------------------------------------- In/Out/Uniforms

// IN
in vec4 a_Vertex;

// OUT
out float varLogDepth;

// UNIFORMS
uniform mat4 su_ModelViewProjectionMatrix;

//----------------------------------------------------- VERTEX SHADER
vec4 logarithmicTransform()
{
	vec4 vertexPosClip = su_ModelViewProjectionMatrix * a_Vertex;
	varLogDepth = vertexPosClip.z;
	return vertexPosClip;
}

void main(void)
{
	gl_Position = logarithmicTransform();
}

