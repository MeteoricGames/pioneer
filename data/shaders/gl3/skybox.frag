// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: SKYBOX

in vec3 v_texCoord;
in float v_skyboxFactor;

out vec4 o_FragColor;

uniform samplerCube texture0;

void main( void )
{
    o_FragColor = vec4(texture( texture0, v_texCoord ).xyz * v_skyboxFactor, 1.0);
}
