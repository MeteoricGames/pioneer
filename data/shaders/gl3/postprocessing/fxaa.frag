// EFFECT: POSTPROCESSING FXAA

// FXAA shader, GLSL code adapted from:
// http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA
// Whitepaper describing the technique:
// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf

// IN
in vec2 v_texCoord;
in vec4 v_vertColor;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform sampler2D texture0;
// The inverse of the texture dimensions along X and Y
uniform vec2 u_texCoordOffset;
uniform float u_fxaaSpanMax; // Default: 8.0
uniform float u_fxaaReduceMul; // Default: 1.0/8.0
uniform float u_fxaaReduceMin; // Default: 1.0/128.0

void main() {
  vec3 rgbNW = texture(texture0, v_texCoord.xy + (vec2(-1.0, -1.0) * u_texCoordOffset)).xyz;
  vec3 rgbNE = texture(texture0, v_texCoord.xy + (vec2(+1.0, -1.0) * u_texCoordOffset)).xyz;
  vec3 rgbSW = texture(texture0, v_texCoord.xy + (vec2(-1.0, +1.0) * u_texCoordOffset)).xyz;
  vec3 rgbSE = texture(texture0, v_texCoord.xy + (vec2(+1.0, +1.0) * u_texCoordOffset)).xyz;
  vec3 rgbM  = texture(texture0, v_texCoord.xy).xyz;
	
  vec3 luma = vec3(0.299, 0.587, 0.114);
  float lumaNW = dot(rgbNW, luma);
  float lumaNE = dot(rgbNE, luma);
  float lumaSW = dot(rgbSW, luma);
  float lumaSE = dot(rgbSE, luma);
  float lumaM  = dot( rgbM, luma);
	
  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
  vec2 dir;
  dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
  dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
	
  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * u_fxaaReduceMul), u_fxaaReduceMin);
	  
  float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
  dir = min(vec2(u_fxaaSpanMax,  u_fxaaSpanMax), 
        max(vec2(-u_fxaaSpanMax, -u_fxaaSpanMax), dir * rcpDirMin)) * u_texCoordOffset;
		
  vec3 rgbA = (1.0/2.0) * (
              texture(texture0, v_texCoord.xy + dir * (1.0/3.0 - 0.5)).xyz +
              texture(texture0, v_texCoord.xy + dir * (2.0/3.0 - 0.5)).xyz);
  vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
              texture(texture0, v_texCoord.xy + dir * (0.0/3.0 - 0.5)).xyz +
              texture(texture0, v_texCoord.xy + dir * (3.0/3.0 - 0.5)).xyz);
  float lumaB = dot(rgbB, luma);

  if((lumaB < lumaMin) || (lumaB > lumaMax)){
    o_FragColor.xyz=rgbA;
  } else {
    o_FragColor.xyz=rgbB;
  }
  o_FragColor.a = 1.0;
    
  //o_FragColor *= v_vertColor;
}