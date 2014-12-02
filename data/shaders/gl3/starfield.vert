// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: STARFIELD

in vec4 a_Vertex;
in vec4 a_Color;

out vec4 color;

struct Material {
	vec4 diffuse;
	vec4 emission;
	vec4 specular;
	float shininess;
};

uniform Material material;
uniform mat4 su_ModelViewProjectionMatrix;

void main(void)
{
	gl_Position = su_ModelViewProjectionMatrix * a_Vertex;
	gl_PointSize = 1.0 + pow(a_Color.r,3.0);
	color = a_Color * material.emission;
}
