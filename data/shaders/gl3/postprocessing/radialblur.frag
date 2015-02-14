// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: POSTPROCESSING RADIALBLUR

uniform sampler2D texture0; // scene texture

uniform float u_sampleDist;			// 2.3: spread (size of iterations)
uniform float u_sampleStrength;		// 4.2: blurriness of samples

varying vec2 v_texCoord0;

out vec4 o_FragColor;

void main(void)
{
	float samples[10] = float[](-0.08, -0.05, -0.03, -0.02, -0.01, 0.01, 0.02, 0.03, 0.05, 0.08);
	vec2 dir = 0.5 - v_texCoord0;
	float dist = sqrt(dir.x * dir.x + dir.y * dir.y);
	dir = dir / dist;
	vec4 color = texture(texture0, v_texCoord0);
	vec4 sum = color;
	for (int i = 0; i < 10; i++) {
		sum += texture(texture0, v_texCoord0 + dir * samples[i] * u_sampleDist );
	}
	sum *= 1.0 / 11.0;
	float t = clamp(dist * u_sampleStrength, 0.0, 1.0);

	o_FragColor = mix(color, sum, t);
}
