// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

// EFFECT: POSTPROCESSING HBLUR

in vec2 v_texCoord;
in vec2 v_blurTexCoords[14];

out vec4 o_FragColor;

uniform sampler2D texture0;

void main(void)
{
   o_FragColor = vec4(0);
   o_FragColor += texture(texture0, v_blurTexCoords[ 0])*0.0044299121055113265;
   o_FragColor += texture(texture0, v_blurTexCoords[ 1])*0.00895781211794;
   o_FragColor += texture(texture0, v_blurTexCoords[ 2])*0.0215963866053;
   o_FragColor += texture(texture0, v_blurTexCoords[ 3])*0.0443683338718;
   o_FragColor += texture(texture0, v_blurTexCoords[ 4])*0.0776744219933;
   o_FragColor += texture(texture0, v_blurTexCoords[ 5])*0.115876621105;
   o_FragColor += texture(texture0, v_blurTexCoords[ 6])*0.147308056121;
   o_FragColor += texture(texture0, v_texCoord         )*0.159576912161;
   o_FragColor += texture(texture0, v_blurTexCoords[ 7])*0.147308056121;
   o_FragColor += texture(texture0, v_blurTexCoords[ 8])*0.115876621105;
   o_FragColor += texture(texture0, v_blurTexCoords[ 9])*0.0776744219933;
   o_FragColor += texture(texture0, v_blurTexCoords[10])*0.0443683338718;
   o_FragColor += texture(texture0, v_blurTexCoords[11])*0.0215963866053;
   o_FragColor += texture(texture0, v_blurTexCoords[12])*0.00895781211794;
   o_FragColor += texture(texture0, v_blurTexCoords[13])*0.0044299121055113265;
}
