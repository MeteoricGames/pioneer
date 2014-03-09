uniform sampler2D texture0; // Exhaust gradient texture
uniform sampler2D texture1; // Depth RT
uniform vec3 u_windowSize;

varying vec4 v_vertexColor;
varying vec2 v_texCoord0;

void main(void)
{
	vec2 texcoord = vec2(gl_FragCoord.x / u_windowSize.x, gl_FragCoord.y / u_windowSize.y);
	// trail overlapping depth [0.2 (1 trail), 1.0 (5 trails)] and alpha factor
	vec4 depth = texture2D(texture1, texcoord);
	// convert depth from [0.2, 1.0] to [0.0, 1.0] gradient
	vec2 g = vec2(max(0.0, (depth.r * 1.25) - 0.25), 0.5); 
	// trail gradient
	float s;
	// Conversion: [0.0, 0.5] -> [0.2, 1.0] and [0.5, 1.0] -> [1.0, 0.2]
	if(v_texCoord0.y < 0.5) {
		s = (1.6 * v_texCoord0.y) + 0.2;
		//s = 2 * v_texCoord0.y;
	} else {
		s = 1.8 - (1.6 * v_texCoord0.y);
		//s = 2 * (v_texCoord0.y - 0.5);
	}
	// trail color
	gl_FragColor = texture2D(texture0, g) / (depth.r * 5.0);
	// trail alpha
	gl_FragColor.a = v_vertexColor.a * s;
	
	SetFragDepth();
}
