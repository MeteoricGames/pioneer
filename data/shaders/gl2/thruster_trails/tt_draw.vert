varying vec4 v_vertexColor;
varying vec2 v_texCoord0;

void main(void)
{
	gl_Position = logarithmicTransform();
	v_vertexColor = gl_Color;
	v_texCoord0 = gl_MultiTexCoord0.xy;
}

