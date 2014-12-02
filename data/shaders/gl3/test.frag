// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: TEST

//----------------------------------------------------- In/Out/Uniforms

// IN
in float varLogDepth;

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
	o_FragColor = vec4(1.0, 0.5, 0.25, 1.0);	
	SetFragDepth();
}
