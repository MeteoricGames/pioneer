// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: SECTOR VIEW ICON

//----------------------------------------------------- In/Out/Uniforms

// IN
in float varLogDepth;
in vec2 v_texCoord0;
in vec4 v_vertexColor;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;
uniform sampler2D texture0; // icon 1
//uniform sampler2D texture1; // icon 2

//------------------------------------------------------ FRAGMENT SHADER
void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

void main(void)
{
	o_FragColor = texture(texture0, v_texCoord0) * v_vertexColor;
	SetFragDepth();
}
