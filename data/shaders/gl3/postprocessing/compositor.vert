// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: POSTPROCESSING COMPOSITOR

in vec4 a_Vertex;

out vec2 v_texCoord;

void main(void)
{
   // Clean up inaccuracies
   vec2 Pos = sign(a_Vertex.xy);

   gl_Position = vec4(Pos.xy, 0, 1);
   // Image-space
   v_texCoord.x = 0.5 * (1.0 + Pos.x);
   v_texCoord.y = 0.5 * (1.0 + Pos.y);

}
