// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: TEXTURED FULLSCREEN QUAD

uniform sampler2D texture0;

in vec2 v_texCoord;

out vec4 o_FragColor;

void main(void)
{
    o_FragColor = texture( texture0, v_texCoord );
}
