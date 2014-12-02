// Copyright � 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _BUILTINSHADERS_H_
#define _BUILTINSHADERS_H_

/*
 * OpenGL 3 basic shaders for general usage by the graphics library.
 */
namespace Graphics { namespace GL3 { namespace Shaders {


// Fullscreen Textured Quad
static const char* FullscreenTexturedQuadVS = R"shader(

in vec4 a_Vertex;

out vec2  v_texCoord;

void main(void)
{
   gl_Position = vec4( a_Vertex.xy, 0.0, 1.0 );
   gl_Position = sign( gl_Position );
    
   // Texture coordinate for screen aligned (in correct range):
   v_texCoord = (vec2( gl_Position.x, gl_Position.y ) + vec2( 1.0 ) ) / vec2( 2.0 );
      
}

)shader";

static const char* FullscreenTexturedQuadFS = R"shader(

uniform sampler2D texture0;

in vec2 v_texCoord;

out vec4 o_FragColor;

void main(void)
{
    o_FragColor = texture( texture0, v_texCoord );
}

)shader";

// Vertex color
static const char* VertexColorVS = R"shader(

in vec4 a_Vertex;
#ifdef VERTEX_COLOR
	in vec4 a_Color;
#endif

#ifdef VERTEX_COLOR
	out vec4 vertexColor;
#endif
out float varLogDepth;

uniform mat4 su_ModelViewProjectionMatrix;
#ifdef POINTS
	uniform float pointSize;
#endif

vec4 logarithmicTransform()
{
	vec4 vertexPosClip = su_ModelViewProjectionMatrix * a_Vertex;
	varLogDepth = vertexPosClip.z;
	return vertexPosClip;
}

void main(void)
{
	gl_Position = logarithmicTransform();
	#ifdef VERTEX_COLOR
		vertexColor = a_Color;
	#endif
	#ifdef POINTS
		gl_PointSize = pointSize;
	#endif
}

)shader";

static const char* VertexColorFS = R"shader(

uniform float invLogZfarPlus1;

#ifdef VERTEX_COLOR
	in vec4 vertexColor;
#endif 

in float varLogDepth;

out vec4 o_FragColor;

#ifndef VERTEX_COLOR
	struct Material {
		vec4 diffuse;
		vec4 emission;
		vec4 specular;
		float shininess;
	};

	uniform Material material;
#endif

void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

void main(void)
{
	#ifdef VERTEX_COLOR
		o_FragColor = vertexColor;
	#else
		o_FragColor = material.diffuse;
	#endif

	SetFragDepth();
}

)shader";

static const char* TestVS = R"shader(

in vec4 a_Vertex;

out float varLogDepth;

vec4 logarithmicTransform()
{
	vec4 vertexPosClip = su_ModelViewProjectionMatrix * a_Vertex;
	varLogDepth = vertexPosClip.z;
	return vertexPosClip;
}

void main(void)
{
	gl_Position = logarithmicTransform();
}

)shader";

static const char* TestFS = R"shader(

in float varLogDepth;

out vec4 o_FragColor;

uniform float invLogZfarPlus1;
struct Material {
	vec4 diffuse;
	vec4 emission;
	vec4 specular;
	float shininess;
};
uniform Material material;

void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

void main(void)
{
	o_FragColor = material.diffuse;
	SetFragDepth();
}

)shader";

}}}

#endif
