// Copyright Â© 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: SKYBOX

in vec4 a_Vertex;

out vec3 v_texCoord;
out float v_skyboxFactor;

uniform mat4 su_ModelViewProjectionMatrix;
uniform vec4 u_viewPosition;
uniform float u_skyboxIntensity;

void main( void )
{
    vec3 position = a_Vertex.xyz;
    position += u_viewPosition.xyz;
    gl_Position = su_ModelViewProjectionMatrix * vec4(position, 1.0);
    v_texCoord    = a_Vertex.xyz;    
	v_skyboxFactor = u_skyboxIntensity;
}
