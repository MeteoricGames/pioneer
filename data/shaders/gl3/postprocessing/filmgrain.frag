// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: FILM GRAIN

// A simple film grain effect

// IN
in vec2 v_texCoord;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform sampler2D texture0;
uniform vec2 su_ViewportSize;
uniform float su_GameTime;
uniform float u_mode;

void main()
{
	vec2 uv = gl_FragCoord.xy / su_ViewportSize.xy;

    vec4 color = texture(texture0, uv);

    float strength = 20.0;

    float x = (uv.x + 4.0 ) * (uv.y + 4.0 ) * (su_GameTime * 10.0);
	vec4 grain = vec4(mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005) * strength;

    if(u_mode == 1.0)
    {
    	grain = 1.0 - grain;
		o_FragColor = color * grain;
    }
    else
    {
		o_FragColor = color + grain;
    }
}
