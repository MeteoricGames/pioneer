// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

varying vec2 v_texCoord0;

void main(void)
{
	gl_Position = logarithmicTransform();	
	v_texCoord0 = gl_MultiTexCoord0.xy;
}
