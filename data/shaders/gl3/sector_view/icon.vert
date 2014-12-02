// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: SECTOR VIEW ICON

//----------------------------------------------------- In/Out/Uniforms

// IN
in vec4 a_Vertex;
in vec4 a_Color;
in vec2 a_MultiTexCoord0;

// OUT
out float varLogDepth;
out vec2 v_texCoord0;
out vec4 v_vertexColor;

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
	v_texCoord0 = a_MultiTexCoord0.xy;
	v_vertexColor = a_Color;
}
