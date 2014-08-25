// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

uniform sampler2D texture0; // Exhaust gradient texture
uniform sampler2D texture1; // Depth RT
uniform vec3 u_windowSize;

varying vec4 v_vertexColor;
varying vec2 v_texCoord0;

void main(void)
{
	// Hot color is used for the first gradient section: whole trail is lit with the hottest color
	vec4 hot_color = texture2D(texture0, vec2(0.5, 0.0)) * v_vertexColor.r;
	hot_color.a = 1.0;
	
	vec2 texcoord = vec2(gl_FragCoord.x / u_windowSize.x, gl_FragCoord.y / u_windowSize.y);
	// trail overlapping depth [0.2 (1 trail), 1.0 (5 trails)] and alpha factor
	vec4 depth = texture2D(texture1, texcoord);
	// convert depth from [0.2, 1.0] to [0.0, 1.0] gradient
	vec2 g = vec2(max(0.0, (depth.r * 1.25) - 0.25), 0.5); 
	// trail gradient
	float s;
	// Conversion: [0.0, 0.5] -> [0.2, 1.0] and [0.5, 1.0] -> [1.0, 0.2]
	if(v_texCoord0.y < 0.5) {
		//s = (1.6 * v_texCoord0.y) + 0.2;			// [0.0, 0.5] -> [0.2, 1.0]
		//s = min((1.6 * v_texCoord0.y) + 0.2, 1.0);// [0.0, 0.5] -> [0.2, 1.0]
		s = min(2.4 * v_texCoord0.y, 1.0);			// [0.0, 0.5] -> [0.0, 1.2]
	} else {
		//s = 1.8 - (1.6 * v_texCoord0.y);			// [0.5, 1.0] -> [1.0, 0.2]
		//s = min(2.2 - (2 * v_texCoord0.y), 1.0);	// [0.5, 1.0] -> [1.2, 0.2]
		s = min(2.4 - (2.4 * v_texCoord0.y), 1.0);	// [0.5, 1.0] -> [1.2, 0.0]
	}
	// trail color
	vec4 trail_color = texture2D(texture0, g) / (depth.r * 5.0);
	
	// trail alpha
	gl_FragColor = (hot_color) + (trail_color * (1.0 - v_vertexColor.r));
	gl_FragColor.a = (v_vertexColor.a * s);
	
	// Testing
	//gl_FragColor = hot_color;
	//gl_FragColor.a = v_vertexColor.r;
	
	SetFragDepth();
}
