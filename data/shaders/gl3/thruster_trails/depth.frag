// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: THRUSTER TRAILS DEPTH

//----------------------------------------------------- In/Out/Uniforms

// IN
in float varLogDepth;
in vec2 v_texCoord0;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;

//------------------------------------------------------ FRAGMENT SHADER
void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

void main(void)
{
	o_FragColor = vec4(1.0, 1.0, 1.0, 0.2);
	SetFragDepth();
}
