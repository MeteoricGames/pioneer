// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: Scanlines

// IN
in vec2 v_texCoord;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform sampler2D texture0;
uniform vec2 su_ViewportSize;

// Emulated input resolution.
// Fixed resolution example: vec2(320.0 / 1.0, 160.0 / 1.0)
uniform vec2 u_resolution;
// Hardness of scanline.
//  -8.0 = soft
// -16.0 = medium
uniform float u_hardScan;
// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
//uniform float u_hardPix;
// Display warp.
// 0.0 = none
// 1.0/8.0 = extreme
uniform vec2 u_warp;
// Amount of shadow mask.
uniform float u_maskDark;
uniform float u_maskLight;

vec2 custom_resolution;

// Emulated input resolution.
//#if 0
  // Fix resolution to set amount.
//  vec2 custom_resolution = vec2(320.0 / 1.0, 160.0 / 1.0);
//#else
  // Optimize for resize.
  //vec2 custom_resolution = su_ViewportSize.xy / 4.0;
//#endif

//------------------------------------------------------------------------

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c) { return(c <= 0.04045)? c / 12.92 : pow((c + 0.055) / 1.055, 2.4); }
vec3 ToLinear(vec3 c) { return vec3(ToLinear1(c.r), ToLinear1(c.g), ToLinear1(c.b)); }

// Linear to sRGB.
// Assuing using sRGB typed textures this should not be needed.
float ToSrgb1(float c) {return(c < 0.0031308? c * 12.92 : 1.055 * pow(c, 0.41666) - 0.055); }
vec3 ToSrgb(vec3 c) { return vec3(ToSrgb1(c.r), ToSrgb1(c.g), ToSrgb1(c.b)); }

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
vec3 Fetch(vec2 pos, vec2 off) {
	pos = floor(pos * custom_resolution + off) / custom_resolution;
    if(max(abs(pos.x - 0.5), abs(pos.y - 0.5)) > 0.5) {
        return vec3(0.0, 0.0, 0.0);
    }
  	return ToLinear(texture(texture0, pos.xy, -16.0).rgb);
}

// Distance in emulated pixels to nearest texel.
vec2 Dist(vec2 pos) { pos = pos * custom_resolution; return -((pos - floor(pos)) - vec2(0.5)); }

// 1D Gaussian.
float Gaus(float pos, float scale) { return exp2(scale * pos * pos); }

// 3-tap Gaussian filter along horz line.
vec3 Horz3(vec2 pos, float off)
{
	vec3 b = Fetch(pos, vec2(-1.0, off));
	vec3 c = Fetch(pos, vec2( 0.0, off));
	vec3 d = Fetch(pos, vec2( 1.0, off));
	float dst = Dist(pos).x;
	// Convert distance to weight.
	float scale = u_hardScan;
	float wb = Gaus(dst - 1.0, scale);
	float wc = Gaus(dst + 0.0, scale);
	float wd = Gaus(dst + 1.0, scale);
  	// Return filtered sample.
  	return (b * wb + c * wc + d * wd) / (wb + wc + wd);
}

// 5-tap Gaussian filter along horz line.
vec3 Horz5(vec2 pos, float off)
{
	vec3 a = Fetch(pos, vec2(-2.0, off));
  	vec3 b = Fetch(pos, vec2(-1.0, off));
  	vec3 c = Fetch(pos, vec2( 0.0, off));
 	vec3 d = Fetch(pos, vec2( 1.0, off));
  	vec3 e = Fetch(pos, vec2( 2.0, off));
  	float dst = Dist(pos).x;
  	// Convert distance to weight.
  	float scale = u_hardScan;
  	float wa = Gaus(dst - 2.0, scale);
  	float wb = Gaus(dst - 1.0, scale);
  	float wc = Gaus(dst + 0.0, scale);
  	float wd = Gaus(dst + 1.0, scale);
  	float we = Gaus(dst + 2.0, scale);
  	// Return filtered sample.
  	return (a * wa + b * wb + c * wc + d * wd + e * we) / (wa + wb + wc + wd + we);
}

// Return scanline weight.
float Scan(vec2 pos, float off)
{
  	float dst = Dist(pos).y;
  	return Gaus(dst + off, u_hardScan);
}

// Allow nearest three lines to effect pixel.
vec3 Tri(vec2 pos)
{
  	vec3 a = Horz3(pos, -1.0);
  	vec3 b = Horz5(pos, 0.0);
  	vec3 c = Horz3(pos, 1.0);
  	float wa = Scan(pos, -1.0);
  	float wb = Scan(pos, 0.0);
  	float wc = Scan(pos, 1.0);
  	return a * wa + b * wb + c * wc;
}

// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos)
{
    pos = pos * 2.0 - 1.0;
    pos *= vec2(1.0 + (pos.y * pos.y) * u_warp.x, 1.0 + (pos.x * pos.x) * u_warp.y);
    return pos * 0.5 + 0.5;
}

// Shadow mask.
vec3 Mask(vec2 pos)
{
  	pos.x += pos.y * 3.0;
  	vec3 mask = vec3(u_maskDark, u_maskDark, u_maskDark);
  	pos.x = fract(pos.x / 6.0);
  	if(pos.x < 0.333) mask.r = u_maskLight;
  	else if(pos.x < 0.666) mask.g = u_maskLight;
  	else mask.b = u_maskLight;
  	return mask;
}

// Entry.
void main(void){
	custom_resolution = su_ViewportSize.xy / u_resolution;

    vec2 pos = Warp(gl_FragCoord.xy / su_ViewportSize.xy);
    o_FragColor.rgb = Tri(pos) * Mask(gl_FragCoord.xy);
    o_FragColor.a = 1.0;
    o_FragColor.rgb = ToSrgb(o_FragColor.rgb);
}
