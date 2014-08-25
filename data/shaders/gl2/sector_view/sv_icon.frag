// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

uniform sampler2D texture0; // icon 1
//uniform sampler2D texture1; // icon 2

varying vec2 v_texCoord0;
varying vec4 v_vertexColor;

void main(void)
{
	gl_FragColor = texture2D(texture0, v_texCoord0) * v_vertexColor;
	//vec4 t2 = texture2D(texture1, texCoord0) * u_color1;
	//gl_FragColor = mix(t1, t2, t2.a);
	SetFragDepth();
}
