// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// IN
in vec4 a_Vertex;
in vec4 a_Color;
in vec2 a_MultiTexCoord0;

// OUT
out float varLogDepth;
out vec4 v_TexCoord0;
out vec4 v_TexCoord1;
out vec4 v_FrontColor;

// UNIFORMS
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

	v_TexCoord0.st = a_MultiTexCoord0;
	v_TexCoord1 = a_Vertex;

	v_FrontColor = a_Color;
}
