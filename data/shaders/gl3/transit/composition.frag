// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GP

// EFFECT: TRANSIT COMPOSITION

//--------------------------------------------------- IN
in vec2 v_texCoord0;

//--------------------------------------------------- OUT
out vec4 o_FragColor;

//--------------------------------------------------- UNIFORMS
uniform sampler2D texture0; // scene texture
uniform sampler2D texture1; // tunnel texture
uniform float u_sampleDist;			// 2.0: spread (size of iterations)
uniform float u_sampleStrength;		// 3.5: blurriness of samples

void main(void)
{
	float samples[10];
	samples[0] = -0.08;
	samples[1] = -0.05;
	samples[2] = -0.03;
	samples[3] = -0.02;
	samples[4] = -0.01;
	samples[5] = 0.01;
	samples[6] = 0.02;
	samples[7] = 0.03;
	samples[8] = 0.05;
	samples[9] = 0.08;
	
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
   
	o_FragColor = texture(texture1, v_texCoord0) + mix(color, sum, t);
}