uniform sampler2D texture0;

varying vec2 texCoord;

void main(void)
{
    gl_FragColor = texture2D( texture0, texCoord );
}