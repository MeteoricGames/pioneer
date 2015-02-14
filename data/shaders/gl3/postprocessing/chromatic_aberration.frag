// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: CHROMATIC ABERRATION

// A simple chromatic aberration effect

// IN
in vec2 v_texCoord;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform sampler2D texture0;
uniform vec2 su_ViewportSize;
uniform float u_centerBuffer;
uniform float u_aberrationStrength;

void main()
{
	vec2 uv = gl_FragCoord.xy / su_ViewportSize.xy;

	// calculate how far each pixel is from the center of the screen
	vec2 vecDist = uv - ( 0.5 , 0.5 );
	float chrDist = length( vecDist );

	// modify the distance from the center, so that only the edges are affected
	chrDist	-= u_centerBuffer;
	if( chrDist < 0.0 ) chrDist = 0.0;

	//distort the UVs
	vec2 uvR = uv * ( 1.0 + chrDist * 0.02 * u_aberrationStrength ),
		 uvB = uv * ( 1.0 - chrDist * 0.02 * u_aberrationStrength );

	//get the individual channels using the modified UVs
	vec4 c;

	c.x = texture2D( texture0 , uvR ).x;
	c.y = texture2D( texture0 , uv ).y;
	c.z = texture2D( texture0 , uvB ).z;

	o_FragColor = c;
}
