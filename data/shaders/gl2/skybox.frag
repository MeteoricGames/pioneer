// Copyright Â© 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

uniform samplerCube texture0;

varying vec3 v_texCoord;
varying float v_skyboxFactor;

void main( void )
{
    gl_FragColor = vec4(textureCube( texture0, v_texCoord ).xyz * v_skyboxFactor, 1.0);
}
