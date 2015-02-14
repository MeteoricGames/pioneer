// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: TONEMAPPING

// An effect that applies defogging, exposure, gamma, vignette, and blue shift corrections.

// IN
in vec2 v_texCoord;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform sampler2D texture0;
// The amount of fog to remove. min: 0, max: 1, default: 0.01
uniform float u_defog;
// The fog color. default: white
uniform vec4 u_fogColor;
// The exposure adjustment. min: -1, max: 1, default: 0.2
uniform float u_exposure;
// The gamma correction exponent. min: 0.5, max: 2, default: 0.8
uniform float u_gamma;
// The center of vignetting. min: (0,0), max: (1,1), default: (0.5, 0.5)
uniform vec2 u_vignetteCenter;
// The radius of vignetting. min: 0, max: 1, default: 1
uniform float u_vignetteRadius;
// The amount of vignetting. min: -1, max: 1, default: -1
uniform float u_vignetteAmount;
// The amount of blue shift. min: 0, max: 1, default: 0.25
uniform float u_blueShift;


void main()
{
    vec4 c = texture(texture0, v_texCoord);
    c.rgb = max(vec3(0), c.rgb - u_defog * u_fogColor.rgb);
    c.rgb *= pow(2.0, u_exposure);
    c.rgb = pow(c.rgb, vec3(u_gamma));

    vec2 tc = v_texCoord - u_vignetteCenter;
    float v = length(tc) / u_vignetteRadius;
    c.rgb += pow(v, 4) * u_vignetteAmount;

    vec3 d = c.rgb * vec3(1.05, 0.97, 1.27);
    c.rgb = mix(c.rgb, d, u_blueShift);

	o_FragColor = c;
}
