// EFFECT: TEXTURED FULLSCREEN QUAD

uniform sampler2D texture0;

in vec2 v_texCoord;

out vec4 o_FragColor;

void main(void)
{
    o_FragColor = texture( texture0, v_texCoord );
}